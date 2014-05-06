#include "font.hpp"
#include "pugixml/pugixml.hpp"
#include "utils.hpp"

#include <stdexcept>

using namespace pugi;
using namespace std;

namespace Blit
{
   Font::Font() : glyphwidth(0), glyphheight(0) {}

   Font::Font(const string& font)
   {
      string dir = Utils::basedir(font);

      xml_document doc;
      if (!doc.load_file(font.c_str()))
         throw runtime_error(Utils::join("Failed to load font: ", font, "."));

      xml_node glyph       = doc.child("font").child("glyphs");
      char start_ascii = glyph.attribute("startascii").as_int();

      int width   = glyph.attribute("width").as_int();
      int height  = glyph.attribute("height").as_int();
      glyphwidth  = glyph.attribute("glyphwidth").as_int();
      glyphheight = glyph.attribute("glyphheight").as_int();
      const char * source = glyph.attribute("source").value();

      if (!width || !height || !glyphwidth || !glyphheight)
         throw logic_error("Invalid glpyh arguments.");

      SurfaceCache cache;
      Surface surf = cache.from_image(Utils::join(dir, "/", source));

      if (surf.rect().w != width * glyphwidth || surf.rect().h != height * glyphheight)
         throw logic_error("Geometry of font and attributes do not match.");

      for (int y = 0; y < height; y++)
      {
         for (int x = 0; x < width; x++, start_ascii++)
         {
            surf_map[start_ascii] = surf.sub(Rect(Pos(x * glyphwidth, y * glyphheight),
                  glyphwidth, glyphheight));

            surf_map[start_ascii].ignore_camera(true);
         }
      }
   }

   const Surface& Font::surface(char c) const
   {
      auto itr = surf_map.find(c);
      if (itr == surf_map.end())
         throw logic_error(Utils::join("Character '", c, "' not found in font."));

      return itr->second;
   }

   void Font::set_color(Pixel pix)
   {
      for (auto& itr : surf_map)
         itr.second.refill_color(pix);
   }

   void Font::render_msg(RenderTarget& target, const string& str, int x, int y,
         Font::RenderAlignment dir,
         int newline_offset) const
   {
      int orig_x = x;

      auto lines = Utils::split(str, '\n');
      for (auto& line : lines)
      {
         x -= Font::adjust_x(line, dir);
         for (auto c : line)
         {
            const Surface& surf = surface(c);
            target.blit_offset(surf, Rect(), Pos(x, y));
            x += glyphwidth;
         }
         y += glyphheight + newline_offset;
         x = orig_x;
      }
   }

   int Font::adjust_x(const string& str, Font::RenderAlignment dir) const
   {
      if (dir == RenderAlignment::Right)
         return glyphwidth * str.size();
      if (dir == RenderAlignment::Centered)
         return glyphwidth * str.size() / 2;
      else return 0;
   }
   
   void FontCluster::add_font(const string& font, Pos offset, Pixel color, string id)
   {
      auto& fonts = fonts_map[move(id)];

      OffsetFont tmp(font);
      tmp.set_color(color);
      tmp.offset = offset;

      fonts.push_back(move(tmp));
   }

   void FontCluster::set_id(string id)
   {
      current_id = move(id);
   }

   Pos FontCluster::glyph_size() const
   {
      auto itr = fonts_map.find(current_id);
      if (itr == fonts_map.end())
         throw runtime_error(Utils::join("Font ID: ", current_id, " not found in map!"));

      auto& fonts = itr->second;

      auto func_x = [](const OffsetFont& a, const OffsetFont& b) {
         return a.glyph_size().x < b.glyph_size().x;
      };
      auto func_y = [](const OffsetFont& a, const OffsetFont& b) {
         return a.glyph_size().y < b.glyph_size().y;
      };

      auto max_x = max_element(fonts.begin(), fonts.end(), func_x);
      auto max_y = max_element(fonts.begin(), fonts.end(), func_y);

      return {max_x->glyph_size().x, max_y->glyph_size().y};
   }

   void FontCluster::render_msg(RenderTarget& target, const string& msg,
         int x, int y,
         Font::RenderAlignment dir,
         int newline_offset) const
   {
      auto itr = fonts_map.find(current_id);
      if (itr == end(fonts_map))
         throw runtime_error(Utils::join("Font ID: ", current_id, " not found in map!"));

      for (auto& font : itr->second)
         font.render_msg(target, msg, x, y, dir, newline_offset);
   }

   FontCluster::OffsetFont::OffsetFont(const string& font) : Font(font)
   {}

   void FontCluster::OffsetFont::render_msg(RenderTarget& target, const string& msg,
         int x, int y,
         Font::RenderAlignment dir,
         int newline_offset) const
   {
      Font::render_msg(target, msg, x + offset.x, y + offset.y, dir, newline_offset);
   }
}

