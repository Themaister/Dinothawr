#ifndef TILEMAP_HPP__
#define TILEMAP_HPP__

#include "surface.hpp"
#include "pugixml/pugixml.hpp"

#include <string>

namespace Blit
{
   class Tilemap : public Renderable
   {
      public:
         Tilemap() = default;
         Tilemap(const std::string& path);
         Tilemap(Tilemap&&) = default;
         Tilemap& operator=(Tilemap&&) = default;

         std::vector<SurfaceCluster>& layers();
         const std::vector<SurfaceCluster>& layers() const;

         void pos(Pos position);
         void render(RenderTarget& target);

      private:
         std::vector<SurfaceCluster> m_layers;

         std::string dir;

         void add_tileset(std::map<unsigned, Surface>& tiles,
               pugi::xml_node node);
         void add_layer(std::map<unsigned, Surface>& tiles,
               pugi::xml_node node, int tilewidth, int tileheight);
   };
}

#endif

