#include "font.hpp"
#include "pugixml/pugixml.hpp"

#include <stdexcept>

using namespace pugi;

namespace Blit
{
   Font::Font() : glyphwidth(0), glyphheight(0) {}

   Font::Font(const std::string& font)
   {
      auto dir = Utils::basedir(font);

      xml_document doc;
      if (!doc.load_file(font.c_str()))
         throw std::runtime_error(Utils::join("Failed to load font: ", font, "."));

      auto glyph       = doc.child("font").child("glyphs");
      char start_ascii = glyph.attribute("startascii").as_int();

      int width   = glyph.attribute("width").as_int();
      int height  = glyph.attribute("height").as_int();
      glyphwidth  = glyph.attribute("glyphwidth").as_int();
      glyphheight = glyph.attribute("glyphheight").as_int();
      auto source = glyph.attribute("source").value();

      if (!width || !height || !glyphwidth || !glyphheight)
         throw std::logic_error("Invalid glpyh arguments.");

      SurfaceCache cache;
      auto surf = cache.from_image(Utils::join(dir, "/", source));

      if (surf.rect().w != width * glyphwidth || surf.rect().h != height * glyphheight)
         throw std::logic_error("Geometry of font and attributes do not match.");

      for (int y = 0; y < height; y++)
      {
         for (int x = 0; x < width; x++, start_ascii++)
         {
            surf_map[start_ascii] = surf.sub({{x * glyphwidth, y * glyphheight},
                  glyphwidth, glyphheight});

            surf_map[start_ascii].ignore_camera(true);
         }
      }
   }

   const Surface& Font::surface(char c) const
   {
      auto itr = surf_map.find(c);
      if (itr == std::end(surf_map))
         throw std::logic_error(Utils::join("Character '", c, "' not found in font."));

      return itr->second;
   }

   void Font::set_color(Pixel pix)
   {
      for (auto& itr : surf_map)
         itr.second.refill_color(pix);
   }

   void Font::render_msg(RenderTarget& target, const std::string& str, int x, int y,
         Font::RenderAlignment dir,
         int newline_offset) const
   {
      int orig_x = x;

      x -= Font::adjust_x(str, dir);

      for (auto c : str)
      {
         if (c != '\n')
         {
            auto& surf = surface(c);
            target.blit_offset(surf, {}, {x, y});
            x += glyphwidth;
         }
         else
         {
            y += glyphheight + newline_offset;
            x = orig_x;

            x -= Font::adjust_x(str, dir);
         }
      }
   }

   int Font::adjust_x(const std::string& str, Font::RenderAlignment dir) const
   {
      if (dir == RenderAlignment::Right)
         return glyphwidth * str.size();
      if (dir == RenderAlignment::Centered)
         return glyphwidth * str.size() / 2;
      else return 0;
   }
   
   void FontCluster::add_font(const std::string& font, Pos offset, Pixel color, std::string id)
   {
      auto& fonts = fonts_map[std::move(id)];

      OffsetFont tmp{font};
      tmp.set_color(color);
      tmp.offset = offset;

      fonts.push_back(std::move(tmp));
   }

   void FontCluster::set_id(std::string id)
   {
      current_id = std::move(id);
   }

   Pos FontCluster::glyph_size() const
   {
      auto itr = fonts_map.find(current_id);
      if (itr == std::end(fonts_map))
         throw std::runtime_error(Utils::join("Font ID: ", current_id, " not found in map!"));

      auto& fonts = itr->second;

      auto func_x = [](const OffsetFont& a, const OffsetFont& b) {
         return a.glyph_size().x < b.glyph_size().x;
      };
      auto func_y = [](const OffsetFont& a, const OffsetFont& b) {
         return a.glyph_size().y < b.glyph_size().y;
      };

      auto max_x = std::max_element(std::begin(fonts), std::end(fonts), func_x);
      auto max_y = std::max_element(std::begin(fonts), std::end(fonts), func_y);

      return {max_x->glyph_size().x, max_y->glyph_size().y};
   }

   void FontCluster::render_msg(RenderTarget& target, const std::string& msg,
         int x, int y,
         Font::RenderAlignment dir,
         int newline_offset) const
   {
      auto itr = fonts_map.find(current_id);
      if (itr == std::end(fonts_map))
         throw std::runtime_error(Utils::join("Font ID: ", current_id, " not found in map!"));

      for (auto& font : itr->second)
         font.render_msg(target, msg, x, y, dir, newline_offset);
   }

   FontCluster::OffsetFont::OffsetFont(const std::string& font) : Font(font)
   {}

   void FontCluster::OffsetFont::render_msg(RenderTarget& target, const std::string& msg,
         int x, int y,
         Font::RenderAlignment dir,
         int newline_offset) const
   {
      Font::render_msg(target, msg, x + offset.x, y + offset.y, dir, newline_offset);
   }
}

