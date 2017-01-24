#include "surface.hpp"
#include "pugixml/pugixml.hpp"
#include "rpng_front.h"
#include <stdexcept>
#include <stdio.h>
#include <new>

using namespace pugi;

namespace Blit
{
   Surface SurfaceCache::from_image(const std::string& path)
   {
      std::shared_ptr<const Blit::Surface::Data> ptr = cache[path];
      if (ptr)
         return Surface(ptr);
      
      cache[path] = load_image(path);
      return cache[path];
   }

   Surface SurfaceCache::from_sprite(const std::string& path)
   {
      xml_document doc;
      if (!doc.load_file(path.c_str()))
         throw std::runtime_error(Utils::join("Failed to load XML sprite: ", path, "."));

      std::basic_string<char> basedir = Utils::basedir(path);
      std::vector<Surface::Alt> alts;

      pugi::xml_node sprite = doc.child("sprite");
      for (pugi::xml_node face = sprite.child("face"); face; face = face.next_sibling())
      {
         const char *id = face.attribute("id").value();
         std::basic_string<char> path   = Utils::join(basedir, "/", face.attribute("source").value());

         std::shared_ptr<const Blit::Surface::Data> ptr = cache[path];
         if (!ptr)
         {
            cache[path] = load_image(path);
            ptr = cache[path];
         }

         alts.push_back(Surface::Alt{ptr, id});
      }

      return Surface(alts, sprite.attribute("start_id").value());
   }

   std::shared_ptr<const Surface::Data> SurfaceCache::load_image(const std::string& path)
   {
      uint32_t *image = NULL;
      unsigned width  = 0;
      unsigned height = 0;
      bool loaded     = rpng_load_image_argb(path.c_str(), &image, &width, &height);

      if (!loaded)
         throw std::runtime_error(Utils::join("RPNG failed to load image: ", path));

      std::vector<Pixel> pix(width * height);
      for (unsigned i = 0; i < width * height; i++)
      {
         pix[i] = Pixel::ARGB(
               uint8_t(image[i] >> 24),
               uint8_t(image[i] >> 16),
               uint8_t(image[i] >>  8),
               uint8_t(image[i] >>  0));
      }

      free(image);
      return std::make_shared<Surface::Data>(std::move(pix), width, height);
   }
}

