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
      {
         if (first)
         {
            loader.request_vorbis(paths[0]);
            last = 0;
         }
         else
         {
            unsigned index = rand() % paths.size();
            if (index == last)
               index = (index + 1) % paths.size();

            loader.request_vorbis(paths[index]);
            last = index;
         }

         first = false;
      }

      auto ret = loader.flush();
      if (ret)
         current = make_shared<Audio::PCMStream>(ret);
      else
         current.reset();

      if (current)
         mixer.add_stream(current);
   }
}

