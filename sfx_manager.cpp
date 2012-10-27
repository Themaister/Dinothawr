#include "game.hpp"
#include <string>
#include <memory>

namespace Icy
{
   void SFXManager::add_stream(const std::string &ident, const std::string &path)
   {
      effects[ident] = std::make_shared<Audio::VorbisFile>(path);
   }

   void SFXManager::play_sfx(const std::string &ident, float volume) const
   {
      auto sfx = effects.find(ident);
      if (sfx == std::end(effects))
         throw std::runtime_error("Invalid SFX!");

      auto duped = sfx->second->dup();
      duped->volume(volume);
      get_mixer().add_stream(duped);
   }
}

