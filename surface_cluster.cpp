#include "surface.hpp"

namespace Blit
{
   std::vector<SurfaceCluster::Elem>& SurfaceCluster::vec()
   {
      return elems;
   }

   const std::vector<SurfaceCluster::Elem>& SurfaceCluster::vec() const
   {
      return elems;
   }

   void SurfaceCluster::set_transform(std::function<Pos (Pos)> func)
   {
      this->func = func;
   }

   void SurfaceCluster::render(RenderTarget& target) const
   {
      for (auto& surf : elems)
         target.blit_offset(surf.surf, Rect(),
               position + (func ? func(surf.offset) : surf.offset));
   }
}

