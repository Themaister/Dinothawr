#ifndef GAME_HPP__
#define GAME_HPP__

#include "surface.hpp"
#include "tilemap.hpp"

#include <string>
#include <functional>
#include <cstddef>

namespace Icy
{
   enum class Input : unsigned
   {
      Up,
      Down,
      Left,
      Right,
      Push
   };

   class Game
   {
      public:
         Game(const std::string& level);

         void input_cb(std::function<bool (Input)> cb) { m_input_cb = cb; }
         void video_cb(std::function<void (const void*, unsigned, unsigned, std::size_t)> cb) { m_video_cb = cb; }

         int width() const { return map.pix_width(); }
         int height() const { return map.pix_height(); }

         void iterate();

      private:
         Blit::Tilemap map;
         Blit::RenderTarget target;
         Blit::Surface player;
         Blit::SurfaceCache cache;

         std::function<bool (Input)> m_input_cb;
         std::function<void (const void*, unsigned, unsigned, std::size_t)> m_video_cb;

         void update_player();
         void move_if_no_collision(Blit::Pos offset);
   };
}

#endif

