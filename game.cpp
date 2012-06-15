#include "game.hpp"
#include "utils.hpp"
#include <iostream>
#include <stdexcept>

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

      run_stepper();

      if (!stepper)
         update_input();
   }

   void Game::update_input()
   {
      if (m_input_cb(Input::Push))
         push_block();
      else if (m_input_cb(Input::Up))
         move_if_no_collision(Input::Up);
      else if (m_input_cb(Input::Down))
         move_if_no_collision(Input::Down);
      else if (m_input_cb(Input::Left))
         move_if_no_collision(Input::Left);
      else if (m_input_cb(Input::Right))
         move_if_no_collision(Input::Right);
   }

   Blit::Pos Game::input_to_offset(Input input)
   {
      switch (input)
      {
         case Input::Up:    return {0, -1};
         case Input::Left:  return {-1, 0};
         case Input::Right: return {1, 0};
         case Input::Down:  return {0, 1};
         default:           return {};
      }
   }

   bool Game::is_offset_collision(Surface& surf, Pos offset)
   {
      auto new_rect = surf.rect() + offset;

      bool outside_grid = surf.rect().pos.x % map.tile_width() || surf.rect().pos.y % map.tile_height();
      if (outside_grid)
         throw std::logic_error("Offset collision check was performed outside tile grid.");

      int current_x = surf.rect().pos.x / map.tile_width();
      int current_y = surf.rect().pos.y / map.tile_height();

      int min_tile_x = new_rect.pos.x / map.tile_width();
      int max_tile_x = (new_rect.pos.x + new_rect.w - 1) / map.tile_width();

      int min_tile_y = new_rect.pos.y / map.tile_height();
      int max_tile_y = (new_rect.pos.y + new_rect.h - 1) / map.tile_height();

      for (int y = min_tile_y; y <= max_tile_y; y++)
         for (int x = min_tile_x; x <= max_tile_x; x++)
            if (Pos{x, y} != Pos{current_x, current_y} && map.collision({x, y})) // Can't collide against ourselves.
               return true;

      return false;
   }

   void Game::push_block()
   {
      auto offset = input_to_offset(facing);
      auto dir    = offset * Pos{map.tile_width(), map.tile_height()};
      auto tile   = map.find_tile("blocks", player.rect().pos + dir);

      if (!tile)
         return;

      int tile_x = player.rect().pos.x / map.tile_width();
      int tile_y = player.rect().pos.y / map.tile_height();
      Pos tile_pos{tile_x, tile_y};

      if (!map.collision(tile_pos + (2 * offset)))
         stepper = std::bind(&Game::tile_stepper, this, std::ref(*tile), offset);
   }

   void Game::move_if_no_collision(Input input)
   {
      facing = input;

      auto offset = input_to_offset(input);
      if (!is_offset_collision(player, offset))
         stepper = std::bind(&Game::tile_stepper, this, std::ref(player), offset);
   }

   bool Game::tile_stepper(Surface& surf, Pos step_dir)
   {
      surf.rect() += step_dir;

      if (surf.rect().pos.x % map.tile_width() || surf.rect().pos.y % map.tile_height())
         return true;

      if (is_offset_collision(surf, step_dir))
         return false;

      //std::cerr << "Player: " << player.rect().pos << " Surf: " << surf->rect().pos << std::endl; 
      auto surface = map.find_tile("floor", surf.rect().pos);
      return surface && surface->attr().find(&surf == &player ? "slippery_player" : "slippery_block") != std::end(surface->attr());
   }

   void Game::run_stepper()
   {
      if (stepper && !stepper())
         stepper = {};
   }
}

