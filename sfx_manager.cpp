#include "game.hpp"
#include <string>
#include <memory>

using namespace std;

namespace Icy
{
   void SFXManager::add_stream(const string &ident, const string &path)
   {
      effects[ident] = make_shared<vector<float>>(Audio::WAVFile::load_wave(path));
   }

   void SFXManager::play_sfx(const string &ident, float volume) const
   {
      auto sfx = effects.find(ident);
      if (sfx == end(effects))
         throw runtime_error("Invalid SFX!");

      auto duped = make_shared<Audio::PCMStream>(sfx->second);
      duped->volume(volume);
      get_mixer().add_stream(duped);
   }
}

