#ifndef GAME_HPP__
#define GAME_HPP__

#include "surface.hpp"
#include "tilemap.hpp"
#include "font.hpp"

#include <string>
#include <functional>
#include <cstddef>
#include <functional>

namespace Icy
{
   enum class Input : unsigned
   {
      Up,
      Down,
      Left,
      Right,
      Push,
      Cancel,
      Start,
      Select,
      Menu,
      None
   };

   class CameraManager
   {
      public:
         CameraManager(Blit::RenderTarget& target, const Blit::Rect& rect, Blit::Pos map_size);
         void update();

      private:
         Blit::RenderTarget* target;
         const Blit::Rect* rect;
         Blit::Pos map_size;
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
         bool won() const;

         static const unsigned fb_width = 320;
         static const unsigned fb_height = 200;

      private:
         Blit::Tilemap map;
         Blit::RenderTarget target;
         Blit::Surface player;
         Blit::SurfaceCache cache;
         Input facing;

         CameraManager camera;

         unsigned won_frame_cnt;
         enum { won_frame_cnt_limit = 60 * 5 };
         bool won_condition();

         std::function<bool (Input)> m_input_cb;
         std::function<void (const void*, unsigned, unsigned, std::size_t)> m_video_cb;

         std::function<bool ()> stepper;
         void run_stepper();

         unsigned frame_cnt;
         bool player_walking;
         bool is_sliding;

         void set_initial_pos(const std::string& level);
         void update_player();
         void update_animation();
         void prepare_won_animation();
         void update_input();
         void move_if_no_collision(Input input);
         void push_block();
         bool is_offset_collision(Blit::Surface& surf, Blit::Pos offset);

         bool tile_stepper(Blit::Surface& surf, Blit::Pos step_dir);
         bool win_animation_stepper();

         static Blit::Pos input_to_offset(Input input);
         std::string input_to_string(Input input);
         Input string_to_input(const std::string& dir);

         std::vector<std::reference_wrapper<Blit::SurfaceCluster::Elem>> get_tiles_with_attr(const std::string& layer,
               const std::string& attr, const std::string& val = "");
   };

   class GameManager
   {
      public:

         enum class State
         {
            Title,
            Menu,
            Game
         };

         GameManager(const std::string& path_game,
               std::function<bool (Input)> input_cb,
               std::function<void (const void*, unsigned, unsigned, std::size_t)> video_cb);

         GameManager();
         GameManager(GameManager&&) = default;
         GameManager& operator=(GameManager&&) = default;

         void input_cb(std::function<bool (Input)> cb) { m_input_cb = cb; }
         void video_cb(std::function<void (const void*, unsigned, unsigned, std::size_t)> cb) { m_video_cb = cb; }

         void iterate();

         bool done() const;

         void reset_level();
         void change_level(unsigned level);
         unsigned current_level() const { return m_current_level; }
         State game_state() const { return m_game_state; }

      private:
         std::vector<std::string> levels;
         std::unique_ptr<Game> game;
         std::string dir;
         unsigned m_current_level;
         State m_game_state;

         Blit::SurfaceCache cache;
         Blit::RenderTarget target;

         Blit::RenderTarget font_bg;
         Blit::FontCluster font_sel;
         Blit::FontCluster font_unsel;

         std::function<bool (Input)> m_input_cb;
         std::function<void (const void*, unsigned, unsigned, std::size_t)> m_video_cb;

         void init_menu(const std::string& title);
         void init_level(unsigned level);

         void step_title();
         void step_game();

         void enter_menu();
         void step_menu();
         int level_select;
         bool old_pressed_menu_up;
         bool old_pressed_menu_down;
   };
}

#endif

