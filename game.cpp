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

      run_stepper();

      if (!stepper)
         update_input();
   }

   void Game::update_input()
   {
      if (m_input_cb(Input::Up))
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

   bool Game::is_offset_collision(Blit::Pos offset)
   {
      auto new_rect = player.rect() + offset;

      int min_tile_x = new_rect.pos.x / map.tile_width();
      int max_tile_x = (new_rect.pos.x + new_rect.w - 1) / map.tile_width();

      int min_tile_y = new_rect.pos.y / map.tile_height();
      int max_tile_y = (new_rect.pos.y + new_rect.h - 1) / map.tile_height();

      for (int y = min_tile_y; y <= max_tile_y; y++)
         for (int x = min_tile_x; x <= max_tile_x; x++)
            if (map.collision({x, y}))
               return true;

      return false;
   }

   void Game::move_if_no_collision(Input input)
   {
      auto offset    = input_to_offset(input);
      bool collision = is_offset_collision(offset);

      if (!collision)
      {
         stepper = std::bind(&Game::tile_stepper, this);
         step_dir = offset;
      }
   }

   bool Game::tile_stepper()
   {
      player.rect() += step_dir;

      bool outside_tile_grid = player.rect().pos.x % map.tile_width() || player.rect().pos.y % map.tile_height();
      bool is_collision = is_offset_collision(step_dir);

      auto surf = map.find_tile(0, player.rect().pos);

      bool slippery = false;
      if (surf)
      {
         auto& attr = surf->attr();
         auto elem  = attr.find("slippery_player");
         if (elem != std::end(attr))
            slippery = elem->second == "true";
      }

      return outside_tile_grid || (slippery && !is_collision);
   }

   void Game::run_stepper()
   {
      if (stepper && !stepper())
         stepper = {};
   }
}

