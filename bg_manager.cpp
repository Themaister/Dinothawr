#include "game.hpp"
#include <cstdlib>

namespace Icy
{
   void BGManager::init(const std::vector<std::string>& paths)
   {
      this->paths = paths;
      std::srand(std::time(nullptr));
   }

   void BGManager::step(Audio::Mixer& mixer)
   {
      if (current && current->valid())
         return;

      if (!paths.size())
         return;

      if (!loader.size())
         loader.request_vorbis(paths[std::rand() % paths.size()]);

      auto ret = loader.flush();
      if (ret)
         current = std::make_shared<Audio::PCMStream>(ret);
      else
         current.reset();

      if (current)
         mixer.add_stream(current);
   }
}

