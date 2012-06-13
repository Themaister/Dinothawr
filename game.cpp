#include "game.hpp"
#include "utils.hpp"

using namespace Blit;

namespace Icy
{
   Game::Game(const std::string& level)
      : map(level), target(map.pix_width(), map.pix_height())
   {
      player = cache.from_image(Utils::join(level, ".png"));
      player.rect().pos = {16, 16};
   }

   void Game::iterate()
   {
      update_player();

      target.clear(Pixel::ARGB(0xff, 0xa0, 0x80, 0x80));
      map.render(target);
      target.blit(player, {});

      target.finalize();

      if (m_video_cb)
         m_video_cb(target.buffer(), target.width(), target.height(), target.width() * sizeof(Pixel));
   }

   void Game::update_player()
   {
      if (!m_input_cb)
         return;

      if (m_input_cb(Input::Up))
         move_if_no_collision({0, -1});
      if (m_input_cb(Input::Down))
         move_if_no_collision({0, 1});
      if (m_input_cb(Input::Left))
         move_if_no_collision({-1, 0});
      if (m_input_cb(Input::Right))
         move_if_no_collision({1, 0});
   }

   void Game::move_if_no_collision(Pos offset)
   {
      auto new_rect = player.rect() + offset;

      int min_tile_x = new_rect.pos.x / map.tile_width();
      int max_tile_x = (new_rect.pos.x + new_rect.w - 1) / map.tile_width();

      int min_tile_y = new_rect.pos.y / map.tile_height();
      int max_tile_y = (new_rect.pos.y + new_rect.h - 1) / map.tile_height();

      for (int y = min_tile_y; y <= max_tile_y; y++)
         for (int x = min_tile_x; x <= max_tile_x; x++)
            if (map.collision({x, y}))
               return;

      player.rect() += offset;
   }
}

