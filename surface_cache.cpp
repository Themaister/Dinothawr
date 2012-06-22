#include "surface.hpp"
#include "pugixml/pugixml.hpp"
#include <png.h>
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
      // Forced to do C-style error handling. libPNG for portability.
      png_structp png_ptr = nullptr;
      png_infop info_ptr  = nullptr;
      png_bytep* rows     = nullptr;
      png_byte header[8];
      unsigned width      = 0;
      unsigned height     = 0;
      png_byte color_type = 0;
      png_byte depth      = 0;

      std::vector<Pixel> buffer;
      Pixel* surf_data = nullptr;

      FILE *file = fopen(path.c_str(), "rb");
      if (!file)
         goto error;

      if (fread(header, 1, sizeof(header), file) < sizeof(header))

      if (png_sig_cmp(header, 0, sizeof(header)))
         goto error;

      png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
      if (!png_ptr)
         goto error;

      info_ptr = png_create_info_struct(png_ptr);
      if (!info_ptr)
         goto error;

      if (setjmp(png_jmpbuf(png_ptr)))
         goto error;

      png_init_io(png_ptr, file);
      png_set_sig_bytes(png_ptr, sizeof(header));
      png_read_info(png_ptr, info_ptr);

      width      = png_get_image_width(png_ptr, info_ptr);
      height     = png_get_image_height(png_ptr, info_ptr);
      color_type = png_get_color_type(png_ptr, info_ptr);
      depth      = png_get_bit_depth(png_ptr, info_ptr);

      if (color_type != PNG_COLOR_TYPE_RGBA)
         goto error;
      if (depth != 8)
         goto error;

      png_set_interlace_handling(png_ptr);
      png_read_update_info(png_ptr, info_ptr);

      rows = new (std::nothrow) png_bytep[height]();
      if (!rows)
         goto error;

      for (unsigned i = 0; i < height; i++)
      {
         rows[i] = new (std::nothrow) png_byte[png_get_rowbytes(png_ptr, info_ptr)]();
         if (!rows[i])
            goto error;
      }

      png_read_image(png_ptr, rows);

      buffer.resize(width * height);
      surf_data = buffer.data();

      for (unsigned y = 0; y < height; y++)
      {
         for (unsigned x = 0; x < width; x++)
         {
            auto ptr   = &rows[y][4 * x];
            unsigned r = ptr[0];
            unsigned g = ptr[1];
            unsigned b = ptr[2];
            unsigned a = ptr[3];
            surf_data[y * width + x] = Pixel::ARGB(a, r, g, b);
         }
      }

      png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
      if (file)
         fclose(file);

      if (rows)
      {
         for (unsigned i = 0; i < height; i++)
            delete[] rows[i];
         delete[] rows;
      }

      return std::make_shared<const Surface::Data>(std::move(buffer), width, height);

error:
      png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
      if (file)
         fclose(file);

      if (rows)
      {
         for (unsigned i = 0; i < height; i++)
            delete[] rows[i];
         delete[] rows;
      }

      throw std::runtime_error("Failed to load image.");
   }
}

