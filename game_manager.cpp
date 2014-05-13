#include "game.hpp"
#include "pugixml/pugixml.hpp"
#include "utils.hpp"

#include <iostream>
#include <cstdlib>
#include <assert.h>

using namespace Blit;
using namespace pugi;
using namespace std;

namespace Icy
{
   GameManager::GameManager(const string& path_game,
         function<bool (Input)> input_cb,
         function<void (const void*, unsigned, unsigned, size_t)> video_cb)
      : save(chapters), dir(Utils::basedir(path_game)),
      m_current_chap(0), m_current_level(0), m_game_state(State::Title),
      m_input_cb(input_cb), m_video_cb(video_cb)
   {
      xml_document doc;

      if (!doc.load_file(path_game.c_str()))
         throw runtime_error(Utils::join("Failed to load game: ", path_game, "."));

      string font_path = Utils::join(dir, "/", doc.child("game").child("font").attribute("source").value());
      font.add_font(font_path, Pos(-1, 1), Pixel::ARGB(0xff, 0xc0, 0x98, 0x00), "yellow");
      font.add_font(font_path, Pos( 0, 0), Pixel::ARGB(0xff, 0xff, 0xde, 0x00), "yellow");
      font.add_font(font_path, Pos(-1, 1), Pixel::ARGB(0xff, 0x73, 0x73, 0x8b), "white");
      font.add_font(font_path, Pos( 0, 0), Pixel::ARGB(0xff, 0xff, 0xff, 0xff), "white");
      font.add_font(font_path, Pos(-1, 1), Pixel::ARGB(0xff, 0x39, 0x5a, 0x94), "lime");
      font.add_font(font_path, Pos( 0, 0), Pixel::ARGB(0xff, 0xb8, 0xe8, 0xb0), "lime");

      init_menu(doc.child("game").child("title").attribute("source").value());
      init_menu_sprite(doc);
      init_sfx(doc);
      init_bg(doc);

      for (xml_node node = doc.child("game").child("chapter"); node; node = node.next_sibling("chapter"))
      {
         Icy::GameManager::Chapter chapter = load_chapter(node, chapters.size());
         if (chapter.num_levels() > 0)
            chapters.push_back(move(chapter));
      }

      ui_target = RenderTarget(Game::fb_width, Game::fb_height);

   }

   GameManager::GameManager() : save(chapters), m_current_chap(0), m_current_level(0), m_game_state(State::Game) {}

   void GameManager::init_menu_sprite(xml_node doc)
   {
      level_complete = cache.from_image(Utils::join(dir, "/", doc.child("game").child("level_complete").attribute("source").value()));

      lock_sprite = cache.from_image(Utils::join(dir, "/", doc.child("game").child("lock_sprite").attribute("source").value()));
      lock_sprite.ignore_camera(true);
      int arrow_x = (Game::fb_width - lock_sprite.rect().w) / 2;
      lock_sprite.rect().pos = Pos( arrow_x, 160 );

      int complete_x = preview_base_x + Game::fb_width / 2 - level_complete.rect().w - 2;
      int complete_y = preview_base_y + Game::fb_height / 2 - level_complete.rect().h - 2;
      level_complete.rect().pos = Pos( complete_x, complete_y );
      level_complete.ignore_camera(true);

      level_select_bg = cache.from_image(Utils::join(dir, "/", doc.child("game").child("menu_bg").attribute("source").value()));
      level_select_bg.ignore_camera(true);

      end_credit_bg = cache.from_image(Utils::join(dir, "/", doc.child("game").child("end_bg").attribute("source").value()));
      end_credit_bg.ignore_camera(true);

      game_bg = cache.from_image(Utils::join(dir, "/", doc.child("game").child("game_bg").attribute("source").value()));
      game_bg.ignore_camera(true);
   }

   void GameManager::init_bg(xml_node doc)
   {
      pugi::xml_node sfx = doc.child("game").child("music");
      Utils::xml_node_walker walk(sfx, "bg", "source");
      vector<BGManager::Track> tracks;
      for (auto& val : walk)
         tracks.push_back({Utils::join(dir, "/", val), 1.0f});

      vector<BGManager::Track>::iterator itr = tracks.begin();
      Utils::xml_node_walker walk_volume(sfx, "bg", "volume");
      for (auto& val : walk_volume)
      {
         itr->gain = val.empty() ? 1.0f : std::strtod(val.c_str(), NULL);
         ++itr;
      }

      get_bg().init(tracks);
   }

   void GameManager::init_sfx(xml_node doc)
   {
      pugi::xml_node sfx = doc.child("game").child("sfx");
      Utils::xml_node_walker walk(sfx, "sound", "name");
      Utils::xml_node_walker walk_source(sfx, "sound", "source");

      vector<pair<string, string> > sfxs;
      for (auto& val : walk)
         sfxs.push_back({val, ""});

      vector<pair<string, string> >::iterator itr = sfxs.begin();
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
      Utils::xml_node_walker walk(chap, "map", "source");
      Utils::xml_node_walker walk_name(chap, "map", "name");

      vector<Level> levels;
      for (auto& val : walk)
         levels.push_back({Utils::join(dir, "/", val), game_bg});

      std::vector<Icy::GameManager::Level>::iterator itr = levels.begin();
      for (auto& val : walk_name)
      {
         itr->set_name(val);
         ++itr;
      }

      int i = 0;
      for (auto& level : levels)
      {
         //cerr << "Found level: " << level.path() << endl;
         level.pos(Pos(preview_base_x + i * preview_delta_x, preview_base_y + preview_delta_y * chapter));

         i++;
      }

      Chapter loaded_chap = Chapter(move(levels), chap.attribute("name").value());
      loaded_chap.set_minimum_clear(chap.attribute("minimum_clear").as_int());
      return move(loaded_chap);
   }

   void GameManager::init_menu(const string& level)
   {
      Surface surf = cache.from_image(Utils::join(dir, "/", level));

      target = RenderTarget(Game::fb_width, Game::fb_height);
      target.blit(surf, Rect());

      font.set_id("yellow");
      font.render_msg(target, "Press OK/Push button", 160, 170, Font::RenderAlignment::Centered);
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
      game->set_bg(game_bg);

      m_current_chap  = chapter;
      m_current_level = level;
   }

   void GameManager::init_level(unsigned chapter, unsigned level)
   {
      change_level(chapter, level);
      m_game_state = State::Game;
   }

   bool GameManager::find_next_unsolved_level(unsigned& current_chap, unsigned& current_level)
   {
      if (current_chap == chapters.size() - 1 && current_level == chapters.back().num_levels() - 1)
         return false;

      unsigned chap = current_chap;
      unsigned level = current_level;
      while (chap < chapters.size())
      {
         if (!chapters[chap].get_completion(level))
         {
            current_chap = chap;
            current_level = level;
            return true;
         }

         level++;
         if (level >= chapters[chap].num_levels())
         {
            if (!chapters[chap].cleared())
               break;

            chap++;
            level = 0;
         }
      }

      return false;
   }

   // Find first level that isn't completed yet, and
   // start menu there.
   void GameManager::set_initial_level()
   {
      save.unserialize();
      m_current_chap = 0;
      m_current_level = 0;
      if (!find_next_unsolved_level(m_current_chap, m_current_level))
      {
         chap_select = chapters.size() - 1;
         level_select = chapters.back().num_levels() - 1;
      }
   }

   void GameManager::step_title()
   {
      if (m_input_cb(Input::Push) || m_input_cb(Input::Menu))
      {
         set_initial_level();
         enter_menu();
      }

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
         unsigned chap = chap_select;
         if (chap < chapters.size() - 1 && !chapters[chap_select].cleared())
            ui_target.blit(lock_sprite, Rect());

         // Render tick if level is complete.
         if (menu_slide_dir.x == 0 && chapters[chap_select].get_completion(level_select))
            ui_target.blit(level_complete, Rect());

         font.set_id("white");
         font.render_msg(ui_target, Utils::join(chap_select + 1,
                  "-", level_select + 1), 240, 155, Font::RenderAlignment::Right);
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

      ui_target.blit(level_select_bg, Rect());

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

      get_sfx().play_sfx("level_next", 0.5);
   }

   void GameManager::step_menu()
   {
      ui_target.blit(level_select_bg, Rect());

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

      int levels_in_chapter = chapters.at(chap_select).num_levels();
      int chapter_size = chapters.size();

      if (pressed_menu_left && !old_pressed_menu_left)
      {
         if (level_select > 0)
         {
            level_select--;
            start_slide({-8, 0}, preview_slide_cnt);
         }
         else if (level_select == 0 && chap_select > 0)
         {
            int levels_in_prev_chapter = chapters[chap_select - 1].num_levels();
            chap_select--;
            start_slide({(levels_in_prev_chapter - 1) * 8, -8}, preview_slide_cnt);
            level_select = levels_in_prev_chapter - 1;
         }
      }
      else if (pressed_menu_right && !old_pressed_menu_right)
      {
         if (level_select < levels_in_chapter - 1)
         {
            level_select++;
            start_slide({8, 0}, preview_slide_cnt);
         }
         else if (level_select == levels_in_chapter - 1 && chap_select < chapter_size - 1)
         {
            if (chapters[chap_select].cleared())
            {
               chap_select++;
               start_slide({(levels_in_chapter - 1) * -8, 8}, preview_slide_cnt);
               level_select = 0;
            }
            else
               get_sfx().play_sfx("chapter_locked", 0.5);
         }
      }
      else if (pressed_menu_up && !old_pressed_menu_up && chap_select > 0)
      {
         int new_level = min(chapters[chap_select - 1].num_levels() - 1, static_cast<unsigned>(level_select));
         chap_select--;
         start_slide({(new_level - static_cast<int>(level_select)) * 8, -8}, preview_slide_cnt);
         level_select = new_level;
      }
      else if (pressed_menu_down && !old_pressed_menu_down && chap_select < static_cast<int>(chapters.size()) - 1)
      {
         if (chapters[chap_select].cleared())
         {
            int new_level = min(chapters[chap_select + 1].num_levels() - 1, static_cast<unsigned>(level_select));
            chap_select++;
            start_slide({(new_level - static_cast<int>(level_select)) * 8, 8}, preview_slide_cnt);
            level_select = new_level;
         }
         else
            get_sfx().play_sfx("chapter_locked", 0.5);
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
      bool pressed_reset = m_input_cb(Input::Reset);

      if (pressed_reset && !old_pressed_reset)
         reset_level();
      else if (pressed_menu && !old_pressed_menu)
         enter_menu();

      old_pressed_menu = pressed_menu;
      old_pressed_reset = pressed_reset;

      if (game->won())
      {
         unsigned pushes = game->get_pushes();
         chapters[m_current_chap].level(m_current_level).set_best_pushes(pushes);

         game.reset();
         bool trigger_completion = !chapters[m_current_chap].get_completion(m_current_level);
         chapters[m_current_chap].set_completion(m_current_level, true);
         save.serialize();

         // Go to ending screen on the event that all levels have been cleared.
         bool cleared_all = trigger_completion;
         if (trigger_completion)
         {
            for (auto& chap : chapters)
            {
               if (chap.cleared_count() != chap.num_levels())
               {
                  cleared_all = false;
                  break;
               }
            }
         }

         if (cleared_all)
         {
            m_game_state = State::End;
            old_pressed_menu_ok = true; // Don't allow us to exit immediately after we enter end credits.
         }
         else
         {
            if (find_next_unsolved_level(m_current_chap, m_current_level))
               change_level(m_current_chap, m_current_level);
            else
               enter_menu();
         }
      }
   }

   void GameManager::step_end()
   {
      ui_target.blit(end_credit_bg, Rect());

      bool pressed_menu_ok = m_input_cb(Input::Push);
      bool trigger_ok = pressed_menu_ok && !old_pressed_menu_ok;
      old_pressed_menu_ok = pressed_menu_ok;
      bool pressed_menu = m_input_cb(Input::Menu);
      bool trigger_menu = pressed_menu && !old_pressed_menu;
      old_pressed_menu = pressed_menu;

      if (trigger_ok || trigger_menu)
         enter_menu();

      font.set_id("white");
      font.render_msg(ui_target, "You completed all levels!\nAwesome! :D\nThanks for playing Dinothawr!", 160, 155, Font::RenderAlignment::Centered, 2);
      m_video_cb(ui_target.buffer(), ui_target.width(), ui_target.height(), ui_target.width() * sizeof(Pixel));
   }

   void GameManager::iterate()
   {
      switch (m_game_state)
      {
         case State::Title: return step_title();
         case State::Menu: return step_menu();
         case State::MenuSlide: return step_menu_slide();
         case State::Game: return step_game();
         case State::End: return step_end();
         default: throw logic_error("Game state is invalid.");
      }
   }

   bool GameManager::done() const
   {
      return false;
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

   GameManager::Level::Level(const string& path, const Blit::Surface& bg)
      : m_path(path), completion(false), best_pushes(0)
   {
      Game game{path};
      game.set_bg(bg);

      static const unsigned scale_factor = 2;
      int preview_width  = Game::fb_width / scale_factor;
      int preview_height = Game::fb_height / scale_factor;

      vector<Pixel> data(preview_width * preview_height);

      game.input_cb([](Input) { return false; });
      game.video_cb([&data, preview_width](const void* pix_data, unsigned width, unsigned height, size_t pitch) {
         const Pixel* pix = reinterpret_cast<const Pixel*>(pix_data);
         pitch /= sizeof(Pixel);

         for (unsigned y = 0; y < height; y += scale_factor)
         {
            for (unsigned x = 0; x < width; x += scale_factor)
            {
               Blit::PixelBase<unsigned int, 8u, 24u, 8u, 16u, 8u, 8u, 8u, 0u> a0 = pix[pitch * (y + 0) + (x + 0)];
               Blit::PixelBase<unsigned int, 8u, 24u, 8u, 16u, 8u, 8u, 8u, 0u> a1 = pix[pitch * (y + 0) + (x + 1)];
               Blit::PixelBase<unsigned int, 8u, 24u, 8u, 16u, 8u, 8u, 8u, 0u> b0 = pix[pitch * (y + 1) + (x + 0)];
               Blit::PixelBase<unsigned int, 8u, 24u, 8u, 16u, 8u, 8u, 8u, 0u> b1 = pix[pitch * (y + 1) + (x + 1)];
               Blit::PixelBase<unsigned int, 8u, 24u, 8u, 16u, 8u, 8u, 8u, 0u> res = Pixel::blend(Pixel::blend(a0, a1), Pixel::blend(b0, b1));

               data[preview_width * (y / scale_factor) + (x / scale_factor)] = res | static_cast<Pixel>(Pixel::alpha_mask);
            }
         }
      });

      game.iterate();

      preview = Surface(make_shared<Surface::Data>(std::move(data), preview_width, preview_height));
      pos(Pos(Game::fb_width, Game::fb_height) / scale_factor - Pos(5, 5));
   }

   void GameManager::Level::render(RenderTarget& target) const
   {
      //preview.rect().pos = position;
      target.blit_offset(preview, Rect(), position); 
   }

   GameManager::SaveManager::SaveManager(vector<GameManager::Chapter> &chaps)
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
      string full_pushes;
      for (auto& chap : chaps)
      {
         string pushes;
         for (auto& level : chap.levels())
            pushes += Utils::join(level.get_best_pushes(), ",");
         full_pushes += pushes + '\n';
      }

      fill(save_data.begin(), save_data.end(), '\0');
      copy(full_pushes.begin(), full_pushes.end(), begin(save_data));
   }

   void GameManager::SaveManager::unserialize()
   {
      string save{save_data.begin(), save_data.end()};

      // Strip trailing zeroes
      long unsigned int last = save.find_last_not_of('\0');
      if (last == string::npos) // Nothing to unserialize ...
         return;

      last++;
      save = save.substr(0, last);

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Dinothawr: Save file: \n%s\n", save.c_str());

      std::vector<std::basic_string<char> > chapters = Utils::split(save, '\n');
      std::vector<Icy::GameManager::Chapter>::iterator chap_itr = chaps.begin();
      for (auto& chap : chapters)
      {
         if (chap_itr == chaps.end())
            return;

         std::vector<std::basic_string<char> > levels = Utils::split(chap, ',');
         std::vector<Icy::GameManager::Level>::iterator level_itr = chap_itr->levels().begin();
         for (auto& level : levels)
         {
            if (level_itr == chap_itr->levels().end())
               break;

            unsigned pushes = Utils::stoi(level);
            level_itr->set_best_pushes(pushes);
            level_itr->set_completion(pushes);
            ++level_itr;
         }

         ++chap_itr;
      }
   }

   size_t GameManager::SaveManager::size() const
   {
      return save_data.size();
   }
}

