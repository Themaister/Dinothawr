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
         Font(Font&&) = default;
         Font& operator=(Font&&) = default;

         const Surface& surface(char c) const; 

         void render_msg(RenderTarget& target, const std::string& msg, int x, int y) const;

      private:
         std::map<char, Surface> surf_map;
         int glyphwidth, glyphheight;
   };
}

#endif

