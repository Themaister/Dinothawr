#include "surface.hpp"
#include <stdexcept>
#include <utility>

namespace Blit
{
   RenderTarget::RenderTarget(int width, int height)
      : m_buffer(width * height), rect({0, 0}, width, height)
   {}

   const Pixel* RenderTarget::buffer() const
   {
      return &m_buffer.front();
   }

   void RenderTarget::clear(Pixel pix)
   {
      std::fill(std::begin(m_buffer), std::end(m_buffer), pix);
   }

   Surface RenderTarget::convert_surface()
   {
      int width = rect.w, height = rect.h;
      rect = {};

      return {std::make_shared<const Surface::Data>(std::move(m_buffer), width, height)};
   }

   int RenderTarget::width() const
   {
      return rect.w;
   }

   int RenderTarget::height() const
   {
      return rect.h;
   }

   Pos RenderTarget::camera_pos() const
   {
      return rect.pos;
   }

   void RenderTarget::camera_move(Pos pos)
   {
      rect.pos += pos;
   }

   void RenderTarget::camera_set(Pos pos)
   {
      rect.pos = pos;
   }

   void RenderTarget::blit(const Surface& surf, Rect subrect)
   {
      blit_offset(surf, subrect, {0, 0});
   }

   void RenderTarget::blit_offset(const Surface& surf_, Rect subrect, Pos pos)
   {
      Surface surf{surf_};
      surf.rect() += pos;

      Rect surf_rect = surf.rect();
      Rect dest_rect = rect;

      bool ignore_camera = surf.ignore_camera();
      if (ignore_camera)
         dest_rect.pos = {0, 0};

      Rect blit_rect = surf_rect & dest_rect;

      if (subrect)
      {
         subrect += surf.rect().pos;
         blit_rect &= subrect;
      }

      if (!blit_rect)
         return;

      auto src_data = surf.pixel_raw(blit_rect.pos);
      auto dst_data = ignore_camera ?
         pixel_raw_no_offset(blit_rect.pos) : pixel_raw(blit_rect.pos);

      for (int y = 0; y < blit_rect.h; y++, src_data += surf_rect.w, dst_data += rect.w)
         Pixel::set_line_if_alpha(dst_data, src_data, blit_rect.w);
   }

   Pixel* RenderTarget::pixel_raw_no_offset(Pos pos)
   {
      int x = pos.x, y = pos.y;

      if (x >= rect.w || y >= rect.h || x < 0 || y < 0)
         throw std::logic_error(Utils::join(
                  "Pixel was fetched out-of-bounds. ",
                  "Asked for: (", x, ", ", y, "). ",
                  "Real dimension: (", rect.w, ", ", rect.h, ")."
                  ));

      return &m_buffer[y * rect.w + x];
   }

   Pixel* RenderTarget::pixel_raw(Pos pos)
   {
      return pixel_raw_no_offset(pos - rect.pos);
   }

   void RenderTarget::finalize()
   {
      for (auto& pix : m_buffer)
         pix &= Pixel::rgb_mask;
   }
}

