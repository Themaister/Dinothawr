#ifndef BLIT_HPP__
#define BLIT_HPP__

#include "utils.hpp"
#include <algorithm>
#include <stdint.h>
#include <vector>

#include <climits>

#if defined(__SSE2__) && defined(USE_SIMD)
#include <emmintrin.h>
#endif

namespace Blit
{
   template <typename T,
            unsigned alpha_bits, unsigned alpha_shift,
            unsigned red_bits,   unsigned red_shift,
            unsigned green_bits, unsigned green_shift,
            unsigned blue_bits,  unsigned blue_shift>
   struct PixelBase
   {
      typedef PixelBase<T,
         alpha_bits, alpha_shift,
         red_bits,   red_shift,
         green_bits, green_shift,
         blue_bits,  blue_shift> self_type;

      typedef T type;
      static const T alpha_mask = ((1u << alpha_bits) - 1) << alpha_shift;
      static const T rgb_mask =
         ((1u << red_bits)   - 1) << red_shift   |
         ((1u << green_bits) - 1) << green_shift |
         ((1u << blue_bits)  - 1) << blue_shift;

#if 0
      static_assert((alpha_bits + red_bits + green_bits + blue_bits) <= CHAR_BIT * sizeof(T),
            "ARGB bitmasks do not match with pixel format.");
      static_assert(alpha_bits + red_bits && green_bits && blue_bits,
            "All colors must have at least 1 bit.");
#endif

      PixelBase(T pixel) : pixel(pixel) {}
      PixelBase() : pixel(0) {}

      operator bool() const { return pixel; }

      self_type operator|(self_type pix) const
      {
         return PixelBase(pixel | pix.pixel);
      }

      self_type operator&(self_type pix) const
      {
         return PixelBase(pixel & pix.pixel);
      }

      self_type& operator|=(self_type pix)
      {
         pixel |= pix.pixel;
         return *this;
      }

      self_type& operator&=(self_type pix)
      {
         pixel &= pix.pixel;
         return *this;
      }

      self_type& set_if_alpha(self_type pix)
      {
         if (pix.pixel & alpha_mask)
            pixel = pix.pixel;

         return *this;
      }

      static self_type ARGB(unsigned a, unsigned r, unsigned g, unsigned b)
      {
         r >>= 8 - red_bits;
         g >>= 8 - green_bits;
         b >>= 8 - blue_bits;

         r <<= red_shift;
         g <<= green_shift;
         b <<= blue_shift;

         a >>= 8 - alpha_bits;
         a <<= alpha_shift;

         return a | r | g | b;
      }

      static void set_line_if_alpha(self_type* dst, const self_type* src, unsigned pix)
      {
         for (unsigned x = 0; x < pix; x++)
            dst[x].set_if_alpha(src[x]);
      }

      static self_type mask_rgb_function(self_type pix)
      {
         return pix & static_cast<self_type>(rgb_mask);
      }

      static void mask_rgb(self_type *dst, std::size_t size)
      {
         std::transform(dst, dst + size, dst, mask_rgb_function);
      }

      template <T shift, T bits>
      T extract_color() const
      {
         return (pixel >> shift) & ((1 << bits) - 1);
      }

      static self_type blend(self_type a, self_type b)
      {
         T a_r = a.extract_color<red_shift, red_bits>();
         T a_g = a.extract_color<green_shift, green_bits>();
         T a_b = a.extract_color<blue_shift, blue_bits>();
         T b_r = b.extract_color<red_shift, red_bits>();
         T b_g = b.extract_color<green_shift, green_bits>();
         T b_b = b.extract_color<blue_shift, blue_bits>();

         T res_r = (a_r + b_r + 1) >> 1;
         T res_g = (a_g + b_g + 1) >> 1;
         T res_b = (a_b + b_b + 1) >> 1;

         return (res_r << red_shift) | (res_g << green_shift) | (res_b << blue_shift);
      }

      T pixel;
   };

   typedef PixelBase<uint32_t,
           8, 24, // A
           8, 16, // R
           8,  8, // G
           8,  0> // B
      Pixel;

   struct Pos
   {
      Pos() : x(0), y(0) {}
      Pos(int x, int y) : x(x), y(y) {}

      Pos& operator+=(Pos pos)       { x += pos.x; y += pos.y; return *this; }
      Pos& operator-=(Pos pos)       { x -= pos.x; y -= pos.y; return *this; }
      Pos& operator*=(Pos pos)       { x *= pos.x; y *= pos.y; return *this; }
      Pos& operator/=(int div)       { x /= div; y /= div; return *this; }
      Pos  operator+ (Pos pos) const { return Pos( x + pos.x, y + pos.y ); }
      Pos  operator- (Pos pos) const { return Pos( x - pos.x, y - pos.y ); }
      Pos  operator* (Pos pos) const { return Pos( x * pos.x, y * pos.y ); }
      Pos  operator/ (int div) const { return Pos( x / div, y / div ); }
      bool operator==(Pos pos) const { return x == pos.x && y == pos.y; }
      bool operator!=(Pos pos) const { return !(*this == pos); }

      // Allows Pos to be placed in binary trees.
      bool operator<(Pos pos) const
      {
         //static_assert(CHAR_BIT * sizeof(int) == 32, "int is not 32-bit. This algorithm will fail.");
         uint64_t self = static_cast<uint32_t>(x);
         self <<= 32;
         self |= static_cast<uint32_t>(y);

         uint64_t other = static_cast<uint32_t>(pos.x);
         other <<= 32;
         other |= static_cast<uint32_t>(pos.y);

         return self < other;
      }

      int x, y;
   };

   inline Pos operator-(Pos pos)
   {
      return Pos(-pos.x, -pos.y);
   }

   inline Pos operator*(int scale, Pos pos)
   {
      return Pos(scale * pos.x, scale * pos.y);
   }

   inline std::ostream& operator<<(std::ostream& ostr, Pos pos)
   {
      ostr << "[ " << pos.x << ", " << pos.y << " ]";
      return ostr;
   }

   struct Rect
   {
      Rect() : w(0), h(0) {}
      Rect(Pos pos, int w, int h) : pos(pos), w(w), h(h) {}
      Rect(int w, int h) : w(w), h(h) {}

      Rect& operator+=(Pos pos)       { this->pos += pos; return *this; }
      Rect& operator-=(Pos pos)       { this->pos -= pos; return *this; }
      Rect  operator+ (Pos pos) const { return Rect( this->pos + pos, w, h ); }
      Rect  operator- (Pos pos) const { return Rect( this->pos - pos, w, h ); }

      // Intersection
      Rect  operator&(Rect rect) const
      {
         int x_left  = std::max(pos.x, rect.pos.x);
         int x_right = std::min(pos.x + w, rect.pos.x + rect.w);
         int width   = x_right - x_left;

         int y_top    = std::max(pos.y, rect.pos.y);
         int y_bottom = std::min(pos.y + h, rect.pos.y + rect.h);
         int height   = y_bottom - y_top;

         if (width <= 0 || height <= 0)
            return Rect(Pos(0, 0), 0, 0);
         else
            return Rect(Pos(x_left, y_top), width, height);
      }

      Rect& operator&=(Rect rect)
      {
         *this = operator&(rect);
         return *this;
      }

      operator bool() const { return w > 0 && h > 0; }

      Pos pos;
      int w, h;
   };
}

#endif

