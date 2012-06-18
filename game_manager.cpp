#include "game.hpp"
#include "pugixml/pugixml.hpp"
#include "utils.hpp"

#include <iostream>

using namespace Blit;
using namespace pugi;

namespace Icy
{
   GameManager::GameManager(const std::string& path_game)
      : dir(Utils::basedir(path_game)), m_current_level(0)
   {
      xml_document doc;
      if (!doc.load_file(path_game.c_str()))
         throw std::runtime_error(Utils::join("Failed to load game: ", path_game, "."));

      Utils::xml_node_walker walk{doc.child("game"), "map", "source"};
      for (auto& val : walk)
         levels.push_back(Utils::join(dir, "/", val));
      
      for (auto& str : levels)
         std::cerr << "Found level: " << str << std::endl;

      reset_level();
   }

   GameManager::GameManager() : m_current_level(0) {}

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

   void GameManager::iterate()
   {
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

   bool GameManager::done() const
   {
      return !game;
   }
}

