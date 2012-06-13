#include "tilemap.hpp"
#include "utils.hpp"

#include <iostream>
#include <stdexcept>
#include <map>
#include <utility>
#include "pugixml/pugixml.hpp"

using namespace pugi;

namespace Blit
{
   Tilemap::Tilemap(const std::string& path) : dir(Utils::basedir(path))
   {
      xml_document doc;
      if (!doc.load_file(path.c_str()))
         throw std::runtime_error(Utils::join("Failed to load XML map: ", path, "."));

      auto map   = doc.child("map");
      width      = map.attribute("width").as_int();
      height     = map.attribute("height").as_int();
      tilewidth  = map.attribute("tilewidth").as_int();
      tileheight = map.attribute("tileheight").as_int();

      if (!width || !height || !tilewidth || !tileheight)
         throw std::logic_error("Tilemap is malformed.");

      std::map<unsigned, Surface> tiles;
      for (auto set = map.child("tileset"); set; set = set.next_sibling("tileset"))
         add_tileset(tiles, set);

      for (auto layer = map.child("layer"); layer; layer = layer.next_sibling("layer"))
      {
         if (Utils::tolower(layer.attribute("name").value()) == "collision")
            add_collision_layer(tiles, layer);
         else
            add_layer(tiles, layer, tilewidth, tileheight);
      }
   }

   void Tilemap::add_tileset(std::map<unsigned, Surface>& tiles, xml_node node)
   {
      int first_gid  = node.attribute("firstgid").as_int();
      int gid_cnt    = 0;
      int tilewidth  = node.attribute("tilewidth").as_int();
      int tileheight = node.attribute("tileheight").as_int();

      auto image     = node.child("image");
      auto source    = image.attribute("source").value();
      int width      = image.attribute("width").as_int();
      int height     = image.attribute("height").as_int();

      //std::cerr << "Adding tileset:" <<
      //   " Gid: " << first_gid <<
      //   " Tilewidth: " << tilewidth <<
      //   " Tileheight: " << tileheight <<
      //   " Source: " << source <<
      //   " Width: " << width <<
      //   " Height: " << height << std::endl;

      if (!width || !height || !tilewidth || !tileheight)
         throw std::logic_error("Tilemap is malformed.");

      auto path = Utils::join(dir, "/", source);

      SurfaceCache cache;
      auto surf = cache.from_image(path);

      if (surf.rect().w != width || surf.rect().h != height)
         throw std::logic_error("Tilemap geometry does not correspond with image values.");

      for (int y = 0; y < height; y += tileheight)
         for (int x = 0; x < width; x += tilewidth)
            tiles[first_gid + gid_cnt++] = surf.sub({{x, y}, tilewidth, tileheight});

      // Load all attributes for a tile into the surface.
      for (auto tile = node.child("tile"); tile; tile = tile.next_sibling("tile"))
      {
         int id = first_gid + tile.attribute("id").as_int();

         for (auto property = tile.child("properties").child("property"); property; property = property.next_sibling())
         {
            auto& attrs = tiles[id].attr();
            auto name   = property.attribute("name").value();
            auto value  = property.attribute("value").value();

            //std::cerr << "Setting attr (" << name << " => " << value << ") to tile #" << id << std::endl;
            attrs[std::move(name)] = std::move(value);
         }
      }
   }

   void Tilemap::add_layer(std::map<unsigned, Surface>& tiles, xml_node node,
         int tilewidth, int tileheight)
   {
      SurfaceCluster cluster;
      int width  = node.attribute("width").as_int();
      int height = node.attribute("height").as_int();

      if (!width || !height)
         throw std::logic_error("Layer is empty.");

      //std::cerr << "Adding layer:" <<
      //   " Width: " << width <<
      //   " Height: " << height << std::endl;

      auto tile = node.child("data").child("tile");
      for (int y = 0; y < height; y++)
      {
         for (int x = 0; x < width; x++, tile = tile.next_sibling("tile"))
         {
            unsigned gid = tile.attribute("gid").as_int();
            if (gid)
               cluster.vec().push_back({tiles[gid], {x * tilewidth, y * tileheight}});
         }
      }

      m_layers.push_back(std::move(cluster));
   }

   void Tilemap::add_collision_layer(std::map<unsigned, Surface>& tiles, xml_node node)
   {
      int width  = node.attribute("width").as_int();
      int height = node.attribute("height").as_int();

      if (!width || !height)
         throw std::logic_error("Layer is empty.");

      auto tile = node.child("data").child("tile");
      for (int y = 0; y < height; y++)
      {
         for (int x = 0; x < width; x++, tile = tile.next_sibling("tile"))
         {
            unsigned gid = tile.attribute("gid").as_int();
            if (gid)
               collisions.insert({x, y});
         }
      }
   }

   void Tilemap::pos(Pos position)
   {
      for (auto& layer : m_layers)
         layer.pos(position);
      Renderable::pos(position);
   }

   void Tilemap::render(RenderTarget& target)
   {
      for (auto& layer : m_layers)
         layer.render(target);
   }

   std::vector<SurfaceCluster>& Tilemap::layers()
   {
      return m_layers;
   }

   const std::vector<SurfaceCluster>& Tilemap::layers() const
   {
      return m_layers;
   }

   bool Tilemap::collision(Pos tile) const
   {
      return collisions.count(tile);
   }
}

