#include "game.hpp"
#include "utils.hpp"
#include <iostream>
#include <stdexcept>

using namespace Blit;
using namespace std;

namespace Icy
{
   EdgeDetector::EdgeDetector(bool init) : pos(init)
   {}

   bool EdgeDetector::set(bool state)
   {
      bool ret = state && !pos;
      pos = state;
      return ret;
   }

   Game::Game(const string& level_path, unsigned chapter, unsigned level, unsigned best_pushes, Blit::FontCluster& font)
      : map(level_path), target(fb_width, fb_height), font(&font),
         camera(target, player.rect(), Pos(map.pix_width(), map.pix_height())),
         won_frame_cnt(0), is_sliding(false), best_pushes(best_pushes), pushes(0), 
         chapter(chapter), level(level), push(true) 
   {
      m_won_early = false;
      set_initial_pos(level_path);
      bg = NULL;
   }

   Game::Game(const string& level_path)
      : map(level_path), target(fb_width, fb_height), font(NULL),
         camera(target, player.rect(), Pos(map.pix_width(), map.pix_height())),
         won_frame_cnt(0), is_sliding(false), push(true) 
   {
      m_won_early = false;
      set_initial_pos(level_path);
      bg = NULL;
   }

   void Game::set_bg(const Blit::Surface& bg)
   {
      this->bg = &bg;
   }

   void Game::set_initial_pos(const string& level)
   {
      Blit::Tilemap::Layer *layer = map.find_layer("floor");
      if (!layer)
         throw runtime_error("Floor layer not found.");

      std::basic_string<char> sprite_path = Utils::find_or_default(layer->attr, "player_sprite", "");
      if (sprite_path.empty())
         player = cache.from_sprite(Utils::join(level, ".sprite"));
      else
         player = cache.from_sprite(Utils::join(Utils::basedir(level), "/", sprite_path));

      int x     = Utils::stoi(Utils::find_or_default(layer->attr, "start_x", "1"));
      int y     = Utils::stoi(Utils::find_or_default(layer->attr, "start_y", "1"));
      int off_x = Utils::stoi(Utils::find_or_default(layer->attr, "player_offset_x", "0"));
      int off_y = Utils::stoi(Utils::find_or_default(layer->attr, "player_offset_y", "0"));
      std::basic_string<char> face = Utils::find_or_default(layer->attr, "start_facing", "right");

      player.rect().pos = Pos(x * map.tile_width(), y * map.tile_height());
      player_off = Pos(off_x, off_y);
      facing = string_to_input(face);
      player.active_alt(face);
   }

   void Game::iterate()
   {
      update_player();

      if (bg)
         target.blit(*bg, Rect());
      else
         target.clear(Pixel::ARGB(0x00, 0x00, 0x00, 0x00));

      camera.update();

      map.render(target);
      target.blit_offset(player, Rect(), player_off);

      if (font)
      {
         font->set_id("lime");
         font->render_msg(target, 
               Utils::join((chapter + 1), "-", (level + 1)), 314, 184, Font::RenderAlignment::Right);
         if (!best_pushes)
            font->render_msg(target, Utils::join(" Pushes:", pushes), 2, 184);
         else
            font->render_msg(target, Utils::join(" Pushes:", pushes, " Best:", best_pushes), 2, 184);
      }

      if (m_video_cb)
         m_video_cb(target.buffer(), target.width(), target.height(), target.width() * sizeof(Pixel));
   }

   vector<reference_wrapper<SurfaceCluster::Elem>> Game::get_tiles_with_attr(const string& name,
         const string& attr, const string& val)
   {
      vector<reference_wrapper<SurfaceCluster::Elem>> surfs;
      Blit::Tilemap::Layer *layer = map.find_layer(name);
      if (!layer)
         return surfs;

      copy_if(layer->cluster.vec().begin(),
            layer->cluster.vec().end(),
            back_inserter(surfs), [&attr, &val](const SurfaceCluster::Elem& surf) -> bool {
               if (val.empty())
                  return surf.surf.attr().find(attr) != surf.surf.attr().end();
               else
                  return Utils::find_or_default(surf.surf.attr(), attr, "") == val; 
            });

      return surfs;
   }

   bool Game::win_animation_stepper()
   {
      won_frame_cnt++;

      std::vector<std::reference_wrapper<Blit::SurfaceCluster::Elem> > goal_blocks = get_tiles_with_attr("blocks", "goal", "true");

      const unsigned frame_per_iter = 24;

      std::string state = "frozen";
      if (won_frame_cnt >= 3 * frame_per_iter)
      {
         bool jump = ((won_frame_cnt / frame_per_iter - 3) >> 1) & 1;
         unsigned last_jump = (((won_frame_cnt - 1) / frame_per_iter - 3) >> 1) & 1;
         state = jump ? "cheer" : "down";
         player.active_alt(state);

         if (jump && !last_jump)
            get_sfx().play_sfx("dino_jump", 0.4);
      }
      else if (won_frame_cnt >= 2 * frame_per_iter)
         state = "defrost2";
      else if (won_frame_cnt >= 1 * frame_per_iter)
         state = "defrost1";

      for (auto& block : goal_blocks)
      {
         block.get().surf.active_alt(state);

         // Shift defrosted block same way player sprite is (16x17, etc), but only when defrost kicks in.
         if (won_frame_cnt >= 1 * frame_per_iter)
            block.get().offset = player_off;
      }

      m_won_early = (won_frame_cnt >= frame_per_iter * 3) && push.set(m_input_cb(Input::Push));
      return true;
   }

   void Game::prepare_won_animation()
   {
      won_frame_cnt = 1;
      player_walking = false;

      push.set(true); // Avoid exiting win animation early.
      m_won_early = false;
      stepper = bind(&Game::win_animation_stepper, this);
      get_sfx().play_sfx("frozen_dino_melt", 0.25);
   }

   bool Game::won() const
   {
      return m_won_early || (won_frame_cnt >= won_frame_cnt_limit);
   }

   static bool is_game_won(const Blit::SurfaceCluster::Elem& a, const Blit::SurfaceCluster::Elem& b)
   {
      return a.surf.rect().pos == b.surf.rect().pos;
   }

   static bool sort_blocks(const SurfaceCluster::Elem& a, const SurfaceCluster::Elem& b)
   {
      return a.surf.rect().pos < b.surf.rect().pos;
   }

   // Checks if all goals on floor and blocks are aligned with each other.
   bool Game::won_condition()
   {
      std::vector<std::reference_wrapper<Blit::SurfaceCluster::Elem> > goal_floor  = get_tiles_with_attr("floor", "goal", "true");
      std::vector<std::reference_wrapper<Blit::SurfaceCluster::Elem> > goal_blocks = get_tiles_with_attr("blocks", "goal", "true");

      if (goal_floor.size() != goal_blocks.size())
         throw logic_error("Number of goal floors and goal blocks do not match.");

      if (goal_floor.empty() || goal_blocks.empty())
         throw logic_error("Goal floor or blocks are empty.");

      sort(goal_floor.begin(), goal_floor.end(), sort_blocks);
      sort(goal_blocks.begin(), goal_blocks.end(), sort_blocks);

      return equal(goal_floor.begin(), goal_floor.end(),
            goal_blocks.begin(), is_game_won);
   }

   void Game::update_player()
   {
      if (!m_input_cb)
         return;
      
      bool had_stepper = static_cast<bool>(stepper);
      run_stepper();

      if (won_frame_cnt)
         return;

      if (!stepper)
         update_input();
      else
         update_triggers();

      // Reset animation.
      if (!had_stepper && stepper)
         frame_cnt = 0;
      else if (!stepper)
      {
         frame_cnt = 0;
         player.active_alt_index(0);
      }

      if (stepper && player_walking)
         update_animation();

      if (won_condition())
         prepare_won_animation();
   }

   void Game::update_animation()
   {
      frame_cnt++;

      // Animation from index 1 to 4, "neutral position" in 0. "Slippery" animations in 5 and 6.
      unsigned anim_index;
      if (is_sliding)
         anim_index = (frame_cnt / 10) % 2 + 5;
      else
         anim_index = (frame_cnt / 10) % 4 + 1;

      player.active_alt_index(anim_index);
   }

   void Game::update_triggers()
   {
      push.set(m_input_cb(Input::Push));
   }

   void Game::update_input()
   {
      bool push_trigger = push.set(m_input_cb(Input::Push));

      if (push_trigger)
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

   string Game::input_to_string(Input input)
   {
      switch (input)
      {
         case Input::Up:    return "up";
         case Input::Left:  return "left";
         case Input::Right: return "right";
         case Input::Down:  return "down";
         default:           return "";
      }
   }

   Blit::Pos Game::input_to_offset(Input input)
   {
      switch (input)
      {
         case Input::Up:    return Pos(0, -1);
         case Input::Left:  return Pos(-1, 0);
         case Input::Right: return Pos(1, 0);
         case Input::Down:  return Pos(0, 1);
         default:           return Pos();
      }
   }

   Input Game::string_to_input(const string& dir)
   {
      if (dir == "up") return Input::Up;
      if (dir == "down") return Input::Down;
      if (dir == "left") return Input::Left;
      if (dir == "right") return Input::Right;
      return Input::None;
   }

   bool Game::is_offset_collision(Surface& surf, Pos offset)
   {
      Blit::Rect new_rect = surf.rect() + offset;

      // Always assume that the rect in question is inside a single tile.
      // This is needed as the dino sprite can be slightly larger than 16x16, but it's
      // *assumed* from a collition detection POV that a surface is tile sized to simplify things.
      new_rect.w = map.tile_width();
      new_rect.h = map.tile_height();

      bool outside_grid = surf.rect().pos.x % map.tile_width() || surf.rect().pos.y % map.tile_height();
      if (outside_grid)
         throw logic_error("Offset collision check was performed outside tile grid.");

      int current_x = surf.rect().pos.x / map.tile_width();
      int current_y = surf.rect().pos.y / map.tile_height();

      int min_tile_x = new_rect.pos.x / map.tile_width();
      int max_tile_x = (new_rect.pos.x + new_rect.w - 1) / map.tile_width();

      int min_tile_y = new_rect.pos.y / map.tile_height();
      int max_tile_y = (new_rect.pos.y + new_rect.h - 1) / map.tile_height();

      for (int y = min_tile_y; y <= max_tile_y; y++)
         for (int x = min_tile_x; x <= max_tile_x; x++)
            if (Pos(x, y) != Pos(current_x, current_y) && map.collision(Pos(x, y))) // Can't collide against ourselves.
               return true;

      return false;
   }

   void Game::push_block()
   {
      Blit::Pos offset = input_to_offset(facing);
      Blit::Pos dir    = offset * Pos(map.tile_width(), map.tile_height());
      Blit::Surface *tile   = map.find_tile("blocks", player.rect().pos + dir);

      if (!tile)
         return;

      int tile_x = player.rect().pos.x / map.tile_width();
      int tile_y = player.rect().pos.y / map.tile_height();
      Pos tile_pos(tile_x, tile_y);

      if (!map.collision(tile_pos + (2 * offset)))
      {
         stepper = bind(&Game::tile_stepper, this, ref(*tile), offset);
         stepper_cnt = 0;
         player_walking = false;
         player.active_alt_index(0);
         get_sfx().play_sfx("dino_push", 1.0);
         pushes++;
      }
   }

   void Game::move_if_no_collision(Input input)
   {
      facing = input;
      player.active_alt(input_to_string(facing));

      Blit::Pos offset = input_to_offset(input);
      if (!is_offset_collision(player, offset))
      {
         stepper = bind(&Game::tile_stepper, this, ref(player), offset);
         player_walking = true;
      }
   }

   bool Game::tile_stepper(Surface& surf, Pos step_dir)
   {
      surf.rect() += 2 * step_dir;

      if (!player_walking)
      {
         unsigned alt = stepper_cnt <= 6 ? 7 : 0;
         player.active_alt_index(alt);
         stepper_cnt++;
      }

      if (surf.rect().pos.x % map.tile_width() || surf.rect().pos.y % map.tile_height())
         return true;

      if (is_offset_collision(surf, step_dir))
      {
         is_sliding = false;

         if (&surf != &player)
            get_sfx().play_sfx("ice_bump", 0.25);

         return false;
      }

      //cerr << "Player: " << player.rect().pos << " Surf: " << surf->rect().pos << endl; 
      Blit::Surface *surface  = map.find_tile("floor", surf.rect().pos);
      bool slippery = surface && Utils::find_or_default(surface->attr(),
            &surf == &player ? "slippery_player" : "slippery_block", "") == "true";

      is_sliding = slippery;
      return slippery;
   }

   void Game::run_stepper()
   {
      if (stepper && !stepper())
         stepper = {};
   }

   CameraManager::CameraManager(RenderTarget& target, const Rect& rect, Blit::Pos map_size)
      : target(&target), rect(&rect), map_size(map_size)
   {}

   void CameraManager::update()
   {
      // Map can fit completely inside our rect, just center it.
      if (target->width() >= map_size.x && target->height() >= map_size.y)
         target->camera_set((map_size - Pos(target->width(), target->height())) / 2);
      else // Center around player, but clamp if player isn't near walls.
      {
         Blit::Pos pos = rect->pos;
         pos += Pos(rect->w, rect->h) / 2;

         Pos target_size(target->width(), target->height());

         Blit::Pos pos_base = pos - target_size / 2;
         Blit::Pos pos_max  = pos_base + target_size;

         if (pos_base.x < 0)
            pos_base.x = 0;
         else if (pos_max.x > map_size.x)
            pos_base.x -= pos_max.x - map_size.x;

         if (pos_base.y < 0)
            pos_base.y = 0;
         else if (pos_max.y > map_size.y)
            pos_base.y -= pos_max.y - map_size.y;

         target->camera_set(pos_base);
      }
   }
}


