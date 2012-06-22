#ifndef FONT_HPP__
#define FONT_HPP__

#include "surface.hpp"
#include <map>

namespace Blit
{
   class Font
   {
      public:
         Font();
         Font(const std::string& font);
         Font(const Font&) = default;
         Font& operator=(const Font&) = default;
         Font(Font&&) = default;
         Font& operator=(Font&&) = default;

         const Surface& surface(char c) const; 
         Pos glyph_size() const { return { glyphwidth, glyphheight }; }

         void render_msg(RenderTarget& target, const std::string& msg, int x, int y, int newline_offset = 0) const;
         void set_color(Pixel pix);

      private:
         std::map<char, Surface> surf_map;
         int glyphwidth, glyphheight;
   };
}

#endif

