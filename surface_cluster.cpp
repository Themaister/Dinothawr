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

   void SurfaceCluster::pos(Pos position)
   {
      for (auto& surf : elems)
         surf.surf.rect().pos = position;

      Renderable::pos(position);
   }

   void SurfaceCluster::set_transform(std::function<Pos (Pos)> func)
   {
      this->func = func;
   }

   void SurfaceCluster::render(RenderTarget& target)
   {
      for (auto& surf : elems)
         target.blit_offset(surf.surf, {},
               func ? func(surf.offset) : surf.offset);
   }
}

