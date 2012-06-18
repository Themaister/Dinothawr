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
      : dir(Utils::basedir(path_game)), m_current_level(0), m_in_menu(true),
         m_input_cb(input_cb), m_video_cb(video_cb)
   {
      xml_document doc;
      if (!doc.load_file(path_game.c_str()))
         throw std::runtime_error(Utils::join("Failed to load game: ", path_game, "."));

      Utils::xml_node_walker walk{doc.child("game"), "map", "source"};
      for (auto& val : walk)
         levels.push_back(Utils::join(dir, "/", val));
      
      for (auto& str : levels)
         std::cerr << "Found level: " << str << std::endl;

      init_menu(doc.child("game").attribute("title").value());
      assert(m_input_cb);
   }

   GameManager::GameManager() : m_current_level(0), m_in_menu(false) {}

   void GameManager::init_menu(const std::string& level)
   {
      Surface surf = cache.from_image(Utils::join(dir, "/", level));

      target = RenderTarget(Game::fb_width, Game::fb_height);
      target.blit(surf, {});

      m_in_menu = true;
   }

   void GameManager::reset_level()
   {
      change_level(m_current_level);
   }

   void GameManager::change_level(unsigned level) 
   {
      game = std::unique_ptr<Game>(new Game(levels.at(level)));
      game->input_cb(m_input_cb);
      game->video_cb(m_video_cb);

      m_current_level = level;
   }

   void GameManager::init_level(unsigned level)
   {
      change_level(level);
      m_in_menu = false;
   }

   void GameManager::step_menu()
   {
      if (m_input_cb(Input::Push))
         init_level(0);

      m_video_cb(target.buffer(), target.width(), target.height(), target.width() * sizeof(Pixel));
   }

   void GameManager::step_game()
   {
      if (!game)
         return;

      game->iterate();

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
      if (m_in_menu)
         step_menu();
      else
         step_game();
   }

   bool GameManager::done() const
   {
      return !game && !m_in_menu;
   }
}

