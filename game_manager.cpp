#include "game.hpp"
#include "pugixml/pugixml.hpp"
#include "utils.hpp"

#include <iostream>
#include <assert.h>

using namespace Blit;
using namespace pugi;

namespace Icy
{
   GameManager::GameManager(const std::string& path_game,
         std::function<bool (Input)> input_cb,
         std::function<void (const void*, unsigned, unsigned, std::size_t)> video_cb)
      : dir(Utils::basedir(path_game)), m_current_level(0), m_game_state(State::Title),
         m_input_cb(input_cb), m_video_cb(video_cb)
   {
      xml_document doc;
      if (!doc.load_file(path_game.c_str()))
         throw std::runtime_error(Utils::join("Failed to load game: ", path_game, "."));

      Utils::xml_node_walker walk{doc.child("game"), "map", "source"};
      Utils::xml_node_walker walk_name{doc.child("game"), "map", "name"};

      for (auto& val : walk)
         levels.push_back(Utils::join(dir, "/", val));

      auto itr = std::begin(levels);
      for (auto& val : walk_name)
      {
         itr->set_name(val);
         ++itr;
      }

      int i = 0;
      for (auto& level : levels)
      {
         std::cerr << "Found level: " << level.path() << std::endl;
         level.pos({preview_base_x + i * preview_delta_x, preview_base_y});

         i++;
      }

      auto font_path = Utils::join(dir, "/", doc.child("game").child("font").attribute("source").value());
      font.add_font(font_path, {-1, 1}, Pixel::ARGB(0xff, 0x3f, 0x3f, 0x00));
      font.add_font(font_path, { 0, 0}, Pixel::ARGB(0xff, 0xff, 0xff, 0x00));

      font_bg = RenderTarget(Game::fb_width, Game::fb_height);

      init_menu(doc.child("game").child("title").attribute("source").value());
   }

   GameManager::GameManager() : m_current_level(0), m_game_state(State::Game) {}

   void GameManager::init_menu(const std::string& level)
   {
      Surface surf = cache.from_image(Utils::join(dir, "/", level));

      target = RenderTarget(Game::fb_width, Game::fb_height);
      target.blit(surf, {});

      font.render_msg(target, "Press Start", 100, 150);
   }

   void GameManager::reset_level()
   {
      change_level(m_current_level);
   }

   void GameManager::change_level(unsigned level) 
   {
      game = Utils::make_unique<Game>(levels.at(level).path());
      game->input_cb(m_input_cb);
      game->video_cb(m_video_cb);

      m_current_level = level;
   }

   void GameManager::init_level(unsigned level)
   {
      change_level(level);
      m_game_state = State::Game;
   }

   void GameManager::step_title()
   {
      if (m_input_cb(Input::Start))
         enter_menu();

      m_video_cb(target.buffer(), target.width(), target.height(), target.width() * sizeof(Pixel));
   }

   void GameManager::enter_menu()
   {
      m_game_state = State::Menu;
      level_select = m_current_level;
      font_bg.camera_set({preview_delta_x * level_select, 0});
   }

   void GameManager::step_menu_slide()
   {
      font_bg.camera_move(menu_slide_dir);
      slide_cnt++;
      if (slide_cnt >= slide_end)
         m_game_state = State::Menu;

      font_bg.clear(Pixel::ARGB(0xff, 0x10, 0x10, 0x10));
      for (auto& preview : levels)
         preview.render(font_bg);

      font.render_msg(font_bg, Utils::join("\"", levels[level_select].name(), "\""), font_preview_base_x, font_preview_base_y);

      m_video_cb(font_bg.buffer(), font_bg.width(), font_bg.height(), font_bg.width() * sizeof(Pixel));
   }

   void GameManager::start_slide(Pos dir)
   {
      m_game_state = State::MenuSlide;

      slide_cnt = 0;
      slide_end = preview_delta_x / std::abs(dir.x);
      menu_slide_dir = dir;
   }

   void GameManager::step_menu()
   {
      font_bg.clear(Pixel::ARGB(0xff, 0x10, 0x10, 0x10));

      for (auto& preview : levels)
         preview.render(font_bg);

      bool pressed_menu_left   = m_input_cb(Input::Left);
      bool pressed_menu_right  = m_input_cb(Input::Right);
      bool pressed_menu_ok     = m_input_cb(Input::Push);
      bool pressed_menu_cancel = m_input_cb(Input::Cancel);

      font.render_msg(font_bg, Utils::join("\"", levels[level_select].name(), "\""), font_preview_base_x, font_preview_base_y);

      if (pressed_menu_left && !old_pressed_menu_right && level_select > 0)
      {
         level_select--;
         start_slide({-8, 0});
      }
      else if (pressed_menu_right && !old_pressed_menu_right && level_select < static_cast<int>(levels.size()) - 1)
      {
         level_select++;
         start_slide({8, 0});
      }

      if (pressed_menu_ok)
         init_level(level_select);
      else if (pressed_menu_cancel && game)
         m_game_state = State::Game;

      old_pressed_menu_left  = pressed_menu_left;
      old_pressed_menu_right = pressed_menu_right;

      m_video_cb(font_bg.buffer(), font_bg.width(), font_bg.height(), font_bg.width() * sizeof(Pixel));
   }

   void GameManager::step_game()
   {
      if (!game)
         return;

      game->iterate();

      if (m_input_cb(Input::Menu))
         enter_menu();

      if (game->won())
      {
         m_current_level++;
         if (m_current_level >= levels.size())
            game.reset();
         else
            change_level(m_current_level);
      }
   }

   void GameManager::iterate()
   {
      switch (m_game_state)
      {
         case State::Title: return step_title();
         case State::Menu: return step_menu();
         case State::MenuSlide: return step_menu_slide();
         case State::Game: return step_game();
         default: throw std::logic_error("Game state is invalid.");
      }
   }

   bool GameManager::done() const
   {
      return !game && m_game_state == State::Game;
   }

   GameManager::Level::Level(const std::string& path)
      : m_path(path)
   {
      Game game{path};

      static const unsigned scale_factor = 2;
      int preview_width  = Game::fb_width / scale_factor;
      int preview_height = Game::fb_height / scale_factor;

      std::vector<Pixel> data(preview_width * preview_height);

      game.input_cb([](Input) { return false; });
      game.video_cb([&data, preview_width](const void* pix_data, unsigned width, unsigned height, size_t pitch) {
               const Pixel* pix = reinterpret_cast<const Pixel*>(pix_data);
               pitch /= sizeof(Pixel);

               for (unsigned y = 0; y < height; y += scale_factor)
               {
                  for (unsigned x = 0; x < width; x += scale_factor)
                  {
                     auto a0 = pix[pitch * (y + 0) + (x + 0)];
                     auto a1 = pix[pitch * (y + 0) + (x + 1)];
                     auto b0 = pix[pitch * (y + 1) + (x + 0)];
                     auto b1 = pix[pitch * (y + 1) + (x + 1)];
                     auto res = Pixel::blend(Pixel::blend(a0, a1), Pixel::blend(b0, b1));
                     
                     data[preview_width * (y / scale_factor) + (x / scale_factor)] = res | static_cast<Pixel>(Pixel::alpha_mask);
                  }
               }
            });

      game.iterate();

      preview = Surface(std::make_shared<const Surface::Data>(std::move(data), preview_width, preview_height));
      pos(Pos{Game::fb_width, Game::fb_height} / scale_factor - Pos{5, 5});
   }

   void GameManager::Level::render(RenderTarget& target)
   {
      preview.rect().pos = position;
      target.blit(preview, {}); 
   }
}

