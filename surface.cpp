#include "surface.hpp"
#include <stdexcept>
#include <utility>
#include <memory>

using namespace std;

namespace Blit
{
   Surface::Surface(Pixel pix, int width, int height)
      : data(make_shared<Data>(pix, width, height)),
      m_active_alt_index(0), m_rect(Pos(0, 0), width, height), m_ignore_camera(false)
   {}

   Surface::Surface(shared_ptr<const Data> data)
      : data(data), m_active_alt_index(0), m_rect(Pos(0, 0), data->w, data->h), m_ignore_camera(false)
   {}

   static bool same_size_func(const vector<Surface::Alt>& alts, Pos size)
   {
      for( vector<Surface::Alt>::const_iterator alt = alts.begin(); alt!=alts.end(); alt++ )
         if(size != Pos(alt->data->w, alt->data->h))
            return false;
      return true;
   }

   Surface::Surface(const vector<Alt>& alts, const string& start_id) : m_ignore_camera(false)
   {
      if (alts.empty())
         throw logic_error("Alts is empty.");

      Pos size(alts.front().data->w, alts.front().data->h);
      m_rect = Rect(Pos(0, 0), size.x, size.y);

      bool same_size = same_size_func(alts,size);

      if (!same_size)
         throw logic_error("Not all alts are of same size.");

      for( vector<Alt>::const_iterator alt = alts.begin(); alt!=alts.end(); alt++ )
         this->alts.insert(std::pair<std::string, std::shared_ptr<const Data>>(alt->tag, alt->data));

      active_alt(start_id);
   }

   void Surface::active_alt(const string& id, unsigned index)
   {
      std::pair
         <std::multimap<std::string, std::shared_ptr<const Data>>::const_iterator,
         std::multimap<std::string, std::shared_ptr<const Data>>::const_iterator>
            itr = alts.equal_range(id);

      iterator_traits<std::multimap<std::string,std::shared_ptr<const Data>>::const_iterator>::
         difference_type dist = distance(itr.first, itr.second);

      if (dist <= static_cast<int>(index))
         throw logic_error(Utils::join("Subindex is out of bounds. Requested Alt: \"", id, "\" Index: ", index));

      advance(itr.first, index);
      std::shared_ptr<const Data> ptr = itr.first->second;
      if (!ptr)
         throw logic_error(Utils::join("Alt ID ", id, " does not exist."));

      m_active_alt = id;
      m_active_alt_index = index;
      data = ptr;
   }

   void Surface::active_alt_index(unsigned index)
   {
      active_alt(m_active_alt, index);
   }

   Surface::Surface()
      : m_rect(Pos(0, 0), 0, 0), m_ignore_camera(false)
   {}

   Surface Surface::sub(Rect rect) const
   {
      RenderTarget target(rect.w, rect.h);
      Surface surf(*this);
      surf.rect().pos = -rect.pos;
      target.blit(surf, rect);
      return target.convert_surface();
   }

   Pixel Surface::pixel(Pos pos) const
   {
      pos -= m_rect.pos;
      int x = pos.x, y = pos.y;

      if (x >= data->w || y >= data->h)
         return 0;

      return data->pixels[y * data->w + x];
   }

   const Pixel* Surface::pixel_raw(Pos pos) const
   {
      pos -= m_rect.pos;
      int x = pos.x, y = pos.y;

      if (x >= data->w || y >= data->h || x < 0 || y < 0)
         throw logic_error(Utils::join(
                  "Pixel was fetched out-of-bounds. ",
                  "Asked for: (", x, ", ", y, "). ",
                  "Real dimension: (", data->w, ", ", data->h, ")."
                  ));

      return &data->pixels[y * data->w + x];
   }

   static Pixel* pixel_ptr = NULL;
   static Pixel transform_func(Pixel old)
   {
      return old & static_cast<Pixel>(Pixel::alpha_mask) ? *pixel_ptr : Pixel();
   }

   void Surface::refill_color(Pixel pixel)
   {
      vector<Pixel> pix;
      pix.reserve(data->w * data->h);

      const std::vector<Pixel>& orig = data->pixels;

      pixel_ptr = &pixel;
      transform(orig.begin(), orig.end(), back_inserter(pix), transform_func);

      data = make_shared<Surface::Data>(move(pix), data->w, data->h);
   }

   void Surface::ignore_camera(bool ignore)
   {
      m_ignore_camera = ignore;
   }

   bool Surface::ignore_camera() const
   {
      return m_ignore_camera;
   }

   Surface::Data::Data(vector<Pixel> pixels, int w, int h)
      : pixels(move(pixels)), w(w), h(h)
   {}

   Surface::Data::Data(Pixel pixel, int w, int h)
      : pixels(w * h)
   {
      fill(pixels.begin(), pixels.end(), pixel);
   }
}

