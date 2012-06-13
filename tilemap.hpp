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

         int tile_width() const { return tilewidth; }
         int tile_height() const { return tileheight; }
         int tiles_width() const { return width; }
         int tiles_height() const { return height; }
         int pix_width() const { return width * tilewidth; }
         int pix_height() const { return height * tileheight; }

      private:
         std::vector<SurfaceCluster> m_layers;

         int width, height, tilewidth, tileheight;
         std::string dir;

         void add_tileset(std::map<unsigned, Surface>& tiles,
               pugi::xml_node node);
         void add_layer(std::map<unsigned, Surface>& tiles,
               pugi::xml_node node, int tilewidth, int tileheight);
   };
}

#endif

