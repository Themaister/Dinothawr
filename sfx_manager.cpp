#include "game.hpp"
#ifndef USE_CXX03
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
      std::map<std::basic_string<char>, std::shared_ptr<std::vector<float> > >::const_iterator sfx = effects.find(ident);
      if (sfx == effects.end())
         throw runtime_error("Invalid SFX!");

      std::shared_ptr<Audio::PCMStream> duped = make_shared<Audio::PCMStream>(sfx->second);
      duped->volume(volume);
      Audio::Mixer& mixer = get_mixer();
      if (mixer.enabled())
         mixer.add_stream(duped);
   }
}
#endif
