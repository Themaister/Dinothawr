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

   class FontCluster
   {
      public:
         FontCluster() = default;
         FontCluster(const FontCluster&) = default;
         FontCluster& operator=(const FontCluster&) = default;
         FontCluster(FontCluster&&) = default;
         FontCluster& operator=(FontCluster&&) = default;

         Pos glyph_size() const;

         void add_font(const std::string& font, Pos offset, Pixel color, std::string id = "");
         void set_id(std::string id);
         void render_msg(RenderTarget& target, const std::string& msg, int x, int y, int newline_offset = 0) const;

      private:
         struct OffsetFont : public Font
         {
            OffsetFont() = default;
            OffsetFont(const std::string& font);
            OffsetFont(const OffsetFont&) = default;
            OffsetFont& operator=(const OffsetFont&) = default;
            OffsetFont(OffsetFont&&) = default;
            OffsetFont& operator=(OffsetFont&&) = default;

            void render_msg(RenderTarget& target, const std::string& msg, int x, int y, int newline_offset = 0) const;
            Pos offset;
         };

         std::map<std::string, std::vector<OffsetFont>> fonts_map;
         std::string current_id;
   };
}

#endif

