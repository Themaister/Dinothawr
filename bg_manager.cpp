#include "game.hpp"
#ifndef USE_CXX03
#include <stdlib.h>

using namespace std;

namespace Icy
{
   void BGManager::init(const vector<Track>& tracks)
   {
      this->tracks = tracks;
      srand(time(NULL));
      first = true;
      last = 0;
   }

   void BGManager::step(Audio::Mixer& mixer)
   {
      lock_guard<Audio::Mixer> guard(mixer);

      if (current && current->valid())
         return;

      if (!tracks.size())
         return;

      if (!loader.size())
      {
         if (first)
         {
            loader.request_vorbis(tracks[0].path);
            last = 0;
         }
         else
         {
            unsigned index = rand() % tracks.size();
            if (index == last)
               index = (index + 1) % tracks.size();

            loader.request_vorbis(tracks[index].path);
            last = index;
         }

         first = false;
      }

      std::shared_ptr<std::vector<float> > ret = loader.flush();

      if (ret)
      {
         current = make_shared<Audio::PCMStream>(ret);
         current->volume(tracks[last].gain);
      }
      else
         current.reset();

      if (current)
         mixer.add_stream(current);
   }
}
#endif
