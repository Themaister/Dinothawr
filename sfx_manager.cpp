#include "game.hpp"
#include <string>
#include <memory>

namespace Icy
{
   void SFXManager::add_stream(const std::string &ident, const std::string &path)
   {
      effects[ident] = std::make_shared<Audio::VorbisFile>(path);
   }

   void SFXManager::play_sfx(const std::string &ident, float volume)
   {
      auto sfx = effects[ident];
      if (!sfx)
         throw std::runtime_error("Invalid SFX!");

      sfx->volume(volume);
      sfx->rewind();
      get_mixer().add_stream(sfx);
   }
}

