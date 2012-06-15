#include "surface.hpp"
#include "pugixml/pugixml.hpp"
#include <Imlib2.h>
#include <stdexcept>

using namespace pugi;

namespace Blit
{
   Surface SurfaceCache::from_image(const std::string& path)
   {
      auto ptr = cache[path];
      if (ptr)
         return {ptr};
      
      cache[path] = load_image(path);
      return cache[path];
   }

   Surface SurfaceCache::from_sprite(const std::string& path)
   {
      xml_document doc;
      if (!doc.load_file(path.c_str()))
         throw std::runtime_error(Utils::join("Failed to load XML sprite: ", path, "."));

      auto basedir = Utils::basedir(path);
      std::vector<Surface::Alt> alts;

      auto sprite = doc.child("sprite");
      for (auto face = sprite.child("face"); face; face = face.next_sibling())
      {
         auto id = face.attribute("id").value();
         auto path   = Utils::join(basedir, "/", face.attribute("source").value());

         auto ptr = cache[path];
         if (!ptr)
         {
            cache[path] = load_image(path);
            ptr = cache[path];
         }

         alts.push_back(Surface::Alt{ptr, id});
      }

      return {alts, sprite.attribute("start_id").value()};
   }

   std::shared_ptr<const Surface::Data> SurfaceCache::load_image(const std::string& path)
   {
      Imlib_Image img = imlib_load_image(path.c_str());
      if (!img)
         throw std::runtime_error(Utils::join("Failed to load image: ", path));

      imlib_context_set_image(img);
      unsigned width  = imlib_image_get_width();
      unsigned height = imlib_image_get_height();
      auto data       = imlib_image_get_data_for_reading_only();

      std::vector<Pixel> buffer(width * height);
      auto surf_data = buffer.data();

      for (unsigned i = 0; i < width * height; i++)
      {
         uint32_t col = data[i]; // ARGB8888
         unsigned a   = (col >> 24) & 0xff;
         unsigned r   = (col >> 16) & 0xff;
         unsigned g   = (col >>  8) & 0xff;
         unsigned b   = (col >>  0) & 0xff;
         surf_data[i] = Pixel::ARGB(a, r, g, b);
      }

      imlib_free_image();

      return std::make_shared<const Surface::Data>(std::move(buffer), width, height);
   }
}

