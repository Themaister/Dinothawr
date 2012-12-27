#include "surface.hpp"
#include "pugixml/pugixml.hpp"
#include "lodepng.h"
#include <stdexcept>
#include <stdio.h>
#include <new>

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
      std::vector<unsigned char> image;
      unsigned width, height;
      unsigned error = lodepng::decode(image, width, height, path);

      if (error)
         throw std::runtime_error(Utils::join("LodePNG error: ", lodepng_error_text(error)));

      std::vector<Pixel> pix(width * height);
      for (unsigned i = 0; i < width * height; i++)
      {
         pix[i] = Pixel::ARGB(
               image[4 * i + 3],
               image[4 * i + 0],
               image[4 * i + 1],
               image[4 * i + 2]);
      }

      return std::make_shared<const Surface::Data>(std::move(pix), width, height);
   }
}

