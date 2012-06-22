#include "surface.hpp"
#include <stdexcept>
#include <utility>
#include <memory>

namespace Blit
{
   Surface::Surface(Pixel pix, int width, int height)
      : data(std::make_shared<const Data>(pix, width, height)),
      m_active_alt_index(0), m_rect({0, 0}, width, height), m_ignore_camera(false)
   {}

   Surface::Surface(std::shared_ptr<const Data> data)
      : data(data), m_active_alt_index(0), m_rect({0, 0}, data->w, data->h), m_ignore_camera(false)
   {}

   Surface::Surface(const std::vector<Alt>& alts, const std::string& start_id) : m_ignore_camera(false)
   {
      if (alts.empty())
         throw std::logic_error("Alts is empty.");

      Pos size{alts.front().data->w, alts.front().data->h};
      m_rect = {{0, 0}, size.x, size.y};

      bool same_size = std::all_of(std::begin(alts) + 1, std::end(alts), [&alts, size](const Alt& alt) {
               return size == Pos{alt.data->w, alt.data->h};
            });

      if (!same_size)
         throw std::logic_error("Not all alts are of same size.");

      for (auto& alt : alts)
         this->alts.insert({alt.tag, alt.data});

      active_alt(start_id);
   }

   void Surface::active_alt(const std::string& id, unsigned index)
   {
      auto itr = alts.equal_range(id);
      auto dist = std::distance(itr.first, itr.second);
      if (dist <= static_cast<int>(index))
         throw std::logic_error(Utils::join("Subindex is out of bounds. Requested Alt: \"", id, "\" Index: ", index));

      std::advance(itr.first, index);
      auto ptr = itr.first->second;
      if (!ptr)
         throw std::logic_error(Utils::join("Alt ID ", id, " does not exist."));

      m_active_alt = id;
      m_active_alt_index = index;
      data = ptr;
   }

   void Surface::active_alt_index(unsigned index)
   {
      active_alt(m_active_alt, index);
   }

   Surface::Surface()
      : m_rect({0, 0}, 0, 0), m_ignore_camera(false)
   {}

   Surface Surface::sub(Rect rect) const
   {
      RenderTarget target(rect.w, rect.h);
      Surface surf{*this};
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
         throw std::logic_error(Utils::join(
                  "Pixel was fetched out-of-bounds. ",
                  "Asked for: (", x, ", ", y, ". ",
                  "Real dimension: ", data->w, ", ", data->h, "."
                  ));

      return &data->pixels[y * data->w + x];
   }

   void Surface::refill_color(Pixel pixel)
   {
      std::vector<Pixel> pix;
      pix.reserve(data->w * data->h);

      auto& orig = data->pixels;
      std::transform(std::begin(orig), std::end(orig), std::back_inserter(pix), [pixel](Pixel old) {
            return old & static_cast<Pixel>(Pixel::alpha_mask) ? pixel : Pixel();
         });

      data = std::make_shared<Surface::Data>(std::move(pix), data->w, data->h);
   }

   void Surface::ignore_camera(bool ignore)
   {
      m_ignore_camera = ignore;
   }

   bool Surface::ignore_camera() const
   {
      return m_ignore_camera;
   }

   Surface::Data::Data(std::vector<Pixel> pixels, int w, int h)
      : pixels(std::move(pixels)), w(w), h(h)
   {}

   Surface::Data::Data(Pixel pixel, int w, int h)
      : pixels(w * h)
   {
      std::fill(std::begin(pixels), std::end(pixels), pixel);
   }
}

