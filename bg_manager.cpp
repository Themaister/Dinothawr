#include "game.hpp"
#include <cstdlib>

using namespace std;

namespace Icy
{
   void BGManager::init(const vector<string>& paths)
   {
      this->paths = paths;
      srand(time(nullptr));
   }

   void BGManager::step(Audio::Mixer& mixer)
   {
      lock_guard<Audio::Mixer> guard(mixer);

      if (current && current->valid())
         return;

      if (!paths.size())
         return;

      if (!loader.size())
         loader.request_vorbis(paths[rand() % paths.size()]);

      auto ret = loader.flush();
      if (ret)
         current = make_shared<Audio::PCMStream>(ret);
      else
         current.reset();

      if (current)
         mixer.add_stream(current);
   }
}

