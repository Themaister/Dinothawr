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
      : save(chapters), dir(Utils::basedir(path_game)),
      m_current_chap(0), m_current_level(0), m_game_state(State::Title),
      m_input_cb(input_cb), m_video_cb(video_cb)
   {
      xml_document doc;
      if (!doc.load_file(path_game.c_str()))
         throw std::runtime_error(Utils::join("Failed to load game: ", path_game, "."));

      for (xml_node node = doc.child("game").child("chapter"); node; node = node.next_sibling("chapter"))
      {
         auto chapter = load_chapter(node, chapters.size());
         if (chapter.num_levels() > 0)
            chapters.push_back(std::move(chapter));
      }

      auto font_path = Utils::join(dir, "/", doc.child("game").child("font").attribute("source").value());
      font.add_font(font_path, {-1, 1}, Pixel::ARGB(0xff, 0xc0, 0x98, 0x00), "yellow");
      font.add_font(font_path, { 0, 0}, Pixel::ARGB(0xff, 0xff, 0xde, 0x00), "yellow");
      font.add_font(font_path, {-1, 1}, Pixel::ARGB(0xff, 0x73, 0x73, 0x8b), "white");
      font.add_font(font_path, { 0, 0}, Pixel::ARGB(0xff, 0xff, 0xff, 0xff), "white");
      font.add_font(font_path, {-1, 1}, Pixel::ARGB(0xff, 0x39, 0x5a, 0x94), "lime");
      font.add_font(font_path, { 0, 0}, Pixel::ARGB(0xff, 0xb8, 0xe8, 0xb0), "lime");

      ui_target = RenderTarget(Game::fb_width, Game::fb_height);

      init_menu(doc.child("game").child("title").attribute("source").value());
      init_menu_sprite(doc);
      init_sfx(doc);
   }

   GameManager::GameManager() : save(chapters), m_current_chap(0), m_current_level(0), m_game_state(State::Game) {}

   void GameManager::init_menu_sprite(xml_node doc)
   {
      auto arrow = cache.from_sprite(Utils::join(dir, "/", doc.child("game").child("arrow").attribute("source").value()));
      arrow.ignore_camera(true);
      arrow_top = arrow;
      arrow_top.active_alt("up");
      arrow_bottom = arrow;

      int arrow_x = (Game::fb_width - arrow.rect().w) / 2;
      arrow_top.rect().pos = { arrow_x, 8 };
      arrow_bottom.rect().pos = { arrow_x, 160 };

      level_complete = cache.from_image(Utils::join(dir, "/", doc.child("game").child("level_complete").attribute("source").value()));

      int complete_x = preview_base_x + Game::fb_width / 2 - level_complete.rect().w - 2;
      int complete_y = preview_base_y + Game::fb_height / 2 - level_complete.rect().h - 2;
      level_complete.rect().pos = { complete_x, complete_y };
      level_complete.ignore_camera(true);

      level_select_bg = cache.from_image(Utils::join(dir, "/", doc.child("game").child("menu_bg").attribute("source").value()));
      level_select_bg.ignore_camera(true);
   }

   void GameManager::init_sfx(xml_node doc)
   {
      auto sfx = doc.child("game").child("sfx");
      Utils::xml_node_walker walk{sfx, "sound", "name"};
      Utils::xml_node_walker walk_source{sfx, "sound", "source"};

      std::vector<std::pair<std::string, std::string>> sfxs;
      for (auto& val : walk)
         sfxs.push_back({val, ""});

      auto itr = std::begin(sfxs);
      for (auto& val : walk_source)
      {
         itr->second = val;
         ++itr;
      }

      for (auto& sfx : sfxs)
         get_sfx().add_stream(sfx.first, Utils::join(dir, "/", sfx.second));
   }

   GameManager::Chapter GameManager::load_chapter(xml_node chap, int chapter)
   {
      Utils::xml_node_walker walk{chap, "map", "source"};
      Utils::xml_node_walker walk_name{chap, "map", "name"};

      std::vector<Level> levels;
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
         //std::cerr << "Found level: " << level.path() << std::endl;
         level.pos({preview_base_x + i * preview_delta_x, preview_base_y + preview_delta_y * chapter});

         i++;
      }

      Chapter loaded_chap = { std::move(levels), chap.attribute("name").value() };
      loaded_chap.set_minimum_clear(chap.attribute("minimum_clear").as_int());
      return std::move(loaded_chap);
   }

   void GameManager::init_menu(const std::string& level)
   {
      Surface surf = cache.from_image(Utils::join(dir, "/", level));

      target = RenderTarget(Game::fb_width, Game::fb_height);
      target.blit(surf, {});

      font.set_id("yellow");
      font.render_msg(target, "Press any button", 136, 170);
   }

   void GameManager::reset_level()
   {
      change_level(m_current_chap, m_current_level);
   }

   void GameManager::change_level(unsigned chapter, unsigned level) 
   {
      game = Utils::make_unique<Game>(
            chapters.at(chapter).level(level).path(), 
            chapter,
            level,
            chapters.at(chapter).level(level).get_best_pushes(),
            font);
      game->input_cb(m_input_cb);
      game->video_cb(m_video_cb);

      m_current_chap  = chapter;
      m_current_level = level;
   }

   void GameManager::init_level(unsigned chapter, unsigned level)
   {
      change_level(chapter, level);
      m_game_state = State::Game;
   }

   void GameManager::step_title()
   {
      if (m_input_cb(Input::Push) ||
            m_input_cb(Input::Menu))
         enter_menu();

      m_video_cb(target.buffer(), target.width(), target.height(), target.width() * sizeof(Pixel));
   }

   void GameManager::enter_menu()
   {
      save.unserialize();
      old_pressed_menu_ok = true; // We don't want to trigger level select right away.
      old_pressed_menu = true;

      m_game_state = State::Menu;
      level_select = m_current_level;
      chap_select  = m_current_chap;
      ui_target.camera_set({preview_delta_x * level_select, preview_delta_y * chap_select});
   }

   void GameManager::menu_render_ui()
   {
      if (menu_slide_dir.y == 0)
      {
         // Render up/down arrows.
         if (chap_select > 0)
            ui_target.blit(arrow_top, {});

         if (static_cast<unsigned>(chap_select) < chapters.size() - 1)
         {
            arrow_bottom.active_alt(chapters[chap_select].cleared() ? "down" : "lock");
            ui_target.blit(arrow_bottom, {});
         }

         // Render tick if level is complete.
         if (menu_slide_dir == Pos{} && chapters[chap_select].get_completion(level_select))
            ui_target.blit(level_complete, {});
      }

      if (menu_slide_dir.y == 0)
      {
         font.set_id("yellow");
         font.render_msg(ui_target, "Chapter", 85, 35);

         font.render_msg(ui_target, Utils::join(chap_select + 1,
                  "/", chapters.size()), 85, 155);

         font.set_id("white");
         font.render_msg(ui_target, "Level", 240, 35, Font::RenderAlignment::Right);
         font.render_msg(ui_target, Utils::join(level_select + 1,
                  "/", chapters[chap_select].num_levels()), 240, 155, Font::RenderAlignment::Right);
      }

      font.set_id("lime");
      font.render_msg(ui_target, Utils::join(total_cleared_levels(),
               "/", total_levels()), 10, 185);

      font.render_msg(ui_target, Utils::join(100 * total_cleared_levels() / total_levels(),
               "%"), 315, 185, Font::RenderAlignment::Right);
   }

   void GameManager::step_menu_slide()
   {
      ui_target.camera_move(menu_slide_dir);
      slide_cnt++;
      if (slide_cnt >= slide_end)
      {
         m_game_state = State::Menu;
         menu_slide_dir = {};
      }

      ui_target.blit(level_select_bg, {});

      for (auto& chap : chapters)
         for (auto& preview : chap.levels())
            preview.render(ui_target);

      menu_render_ui();

      m_video_cb(ui_target.buffer(), ui_target.width(), ui_target.height(), ui_target.width() * sizeof(Pixel));
   }

   const GameManager::Level& GameManager::get_selected_level() const
   {
      return chapters.at(chap_select).level(level_select);
   }

   void GameManager::start_slide(Pos dir, unsigned cnt)
   {
      m_game_state = State::MenuSlide;

      slide_cnt = 0;
      slide_end = cnt;

      menu_slide_dir = dir;

      get_sfx().play_sfx("level_next");
   }

   void GameManager::step_menu()
   {
      ui_target.blit(level_select_bg, {});

      for (auto& chap : chapters)
         for (auto& preview : chap.levels())
            preview.render(ui_target);

      menu_render_ui();

      // Check input. Start menu slide if selecting different level.
      bool pressed_menu_left   = m_input_cb(Input::Left);
      bool pressed_menu_right  = m_input_cb(Input::Right);
      bool pressed_menu_up     = m_input_cb(Input::Up);
      bool pressed_menu_down   = m_input_cb(Input::Down);
      bool pressed_menu_ok     = m_input_cb(Input::Push);
      bool pressed_menu			 = m_input_cb(Input::Menu);

      if (pressed_menu_left && !old_pressed_menu_left && level_select > 0)
      {
         level_select--;
         start_slide({-8, 0}, preview_slide_cnt);
      }
      else if (pressed_menu_right && !old_pressed_menu_right && level_select < static_cast<int>(chapters.at(chap_select).num_levels()) - 1)
      {
         level_select++;
         start_slide({8, 0}, preview_slide_cnt);
      }
      else if (pressed_menu_up && !old_pressed_menu_up && chap_select > 0)
      {
         int new_level = std::min(chapters[chap_select - 1].num_levels() - 1, static_cast<unsigned>(level_select));
         chap_select--;
         start_slide({(new_level - static_cast<int>(level_select)) * 8, -8}, preview_slide_cnt);
         level_select = new_level;
      }
      else if (pressed_menu_down && !old_pressed_menu_down && chap_select < static_cast<int>(chapters.size()) - 1)
      {
         if (chapters[chap_select].cleared())
         {
            int new_level = std::min(chapters[chap_select + 1].num_levels() - 1, static_cast<unsigned>(level_select));
            chap_select++;
            start_slide({(new_level - static_cast<int>(level_select)) * 8, 8}, preview_slide_cnt);
            level_select = new_level;
         }
         else
            get_sfx().play_sfx("chapter_locked");
      }
      else if (pressed_menu_ok && !old_pressed_menu_ok)
         init_level(chap_select, level_select);
      else if (pressed_menu && !old_pressed_menu && game)
         m_game_state = State::Game;

      old_pressed_menu_left   = pressed_menu_left;
      old_pressed_menu_right  = pressed_menu_right;
      old_pressed_menu_up     = pressed_menu_up;
      old_pressed_menu_down   = pressed_menu_down;
      old_pressed_menu_ok     = pressed_menu_ok;
      old_pressed_menu        = pressed_menu;

      m_video_cb(ui_target.buffer(), ui_target.width(), ui_target.height(), ui_target.width() * sizeof(Pixel));
   }

   void GameManager::step_game()
   {
      if (!game)
         return;

      game->iterate();

      bool pressed_menu = m_input_cb(Input::Menu);

      if (pressed_menu && !old_pressed_menu)
         enter_menu();

      old_pressed_menu = pressed_menu;

      if (game->won())
      {
         unsigned pushes = game->get_pushes();
         chapters[m_current_chap].level(m_current_level).set_best_pushes(pushes);

         game.reset();
         chapters[m_current_chap].set_completion(m_current_level, true);
         save.serialize();

         if (m_current_level < chapters.at(m_current_chap).num_levels() - 1)
         {
            m_current_level++;
            change_level(m_current_chap, m_current_level);
         }
         else
            enter_menu();
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

   unsigned GameManager::total_levels() const
   {
      unsigned levels = 0;
      for (auto& chap : chapters)
         levels += chap.num_levels();

      return levels;
   }

   unsigned GameManager::total_cleared_levels() const
   {
      unsigned levels = 0;
      for (auto& chap : chapters)
         levels += chap.cleared_count();

      return levels;
   }

   GameManager::Level::Level(const std::string& path)
      : m_path(path), completion(false)
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

      preview = Surface(std::make_shared<Surface::Data>(std::move(data), preview_width, preview_height));
      pos(Pos{Game::fb_width, Game::fb_height} / scale_factor - Pos{5, 5});
   }

   void GameManager::Level::render(RenderTarget& target) const
   {
      //preview.rect().pos = position;
      target.blit_offset(preview, {}, position); 
   }

   GameManager::SaveManager::SaveManager(std::vector<GameManager::Chapter> &chaps)
      : chaps(chaps)
   {
      save_data.resize(save_game_size);
   }


   void* GameManager::SaveManager::data()
   {
      return save_data.data();
   }

   void GameManager::SaveManager::serialize()
   {
      std::string full_completed;
      for (auto& chap : chaps)
      {
         std::string completed;
         for (auto& level : chap.levels())
            completed += level.get_completion() ? "1," : "0,";
         full_completed += completed + '\n';
      }

      std::fill(std::begin(save_data), std::end(save_data), '\0');
      std::copy(std::begin(full_completed), std::end(full_completed), std::begin(save_data));
   }

   void GameManager::SaveManager::unserialize()
   {
      std::string save{std::begin(save_data), std::end(save_data)};

      // Strip trailing zeroes
      auto last = save.find_last_not_of('\0');
      if (last == std::string::npos) // Nothing to unserialize ...
         return;

      last++;
      save = save.substr(0, last);

      auto chapters = Utils::split(save, '\n');
      auto chap_itr = std::begin(chaps);
      for (auto& chap : chapters)
      {
         if (chap_itr == std::end(chaps))
            return;

         auto levels = Utils::split(chap, ',');
         auto level_itr = std::begin(chap_itr->levels());
         for (auto& level : levels)
         {
            if (level_itr == std::end(chap_itr->levels()))
               break;

            if (level == "1")
               level_itr->set_completion(true);
            else if (level == "0")
               level_itr->set_completion(false);
            else
               throw std::logic_error("Completion state is invalid.");

            ++level_itr;
         }

         ++chap_itr;
      }
   }

   std::size_t GameManager::SaveManager::size() const
   {
      return save_data.size();
   }
}

