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
         add_layer(tiles, layer, tilewidth, tileheight);
   }

   std::map<std::string, std::string> Tilemap::get_attributes(xml_node parent, const std::string& child) const
   {
      std::map<std::string, std::string> attrs;

      for (auto node = parent.child(child.c_str()); node; node = node.next_sibling(child.c_str()))
      {
         auto name = node.attribute("name").value();
         auto value = node.attribute("value").value();
         attrs.insert({name, value});
      }

      return attrs;
   }

   void Tilemap::add_tileset(std::map<unsigned, Surface>& tiles, xml_node node)
   {
      int first_gid  = node.attribute("firstgid").as_int();
      int id_cnt     = 0;
      int tilewidth  = node.attribute("tilewidth").as_int();
      int tileheight = node.attribute("tileheight").as_int();

      auto image     = node.child("image");
      auto source    = image.attribute("source").value();
      int width      = image.attribute("width").as_int();
      int height     = image.attribute("height").as_int();

      std::cerr << "Adding tileset:" <<
         " Name: " << node.attribute("name").value() <<
         " Gid: " << first_gid <<
         " Tilewidth: " << tilewidth <<
         " Tileheight: " << tileheight <<
         " Source: " << source <<
         " Width: " << width <<
         " Height: " << height << std::endl;

      if (!width || !height || !tilewidth || !tileheight)
         throw std::logic_error("Tilemap is malformed.");

      SurfaceCache cache;
      auto surf = cache.from_image(Utils::join(dir, "/", source));

      if (surf.rect().w != width || surf.rect().h != height)
         throw std::logic_error("Tilemap geometry does not correspond with image values.");

      auto global_attr = get_attributes(node.child("properties"), "property");

      std::cerr << "Dumping attrs:" << std::endl;
      for (auto& attr : global_attr)
         std::cerr << "Found global attr (" << attr.first << " => " << attr.second << ")." << std::endl;
      std::cerr << "Dumped attrs." << std::endl;

      for (int y = 0; y < height; y += tileheight)
      {
         for (int x = 0; x < width; x += tilewidth, id_cnt++)
         {
            int id = first_gid + id_cnt;
            tiles[id] = surf.sub({{x, y}, tilewidth, tileheight});
            std::copy(std::begin(global_attr), std::end(global_attr), std::inserter(tiles[id].attr(), std::begin(tiles[id].attr()))); 
         }
      }

      // Load all attributes for a tile into the surface.
      for (auto tile = node.child("tile"); tile; tile = tile.next_sibling("tile"))
      {
         int id = first_gid + tile.attribute("id").as_int();

         auto attrs = get_attributes(tile.child("properties"), "property");
         std::copy(std::begin(global_attr), std::end(global_attr), std::inserter(attrs, std::begin(attrs)));

         auto itr = attrs.find("sprite");

         if (itr != std::end(attrs))
            tiles[id] = cache.from_sprite(Utils::join(dir, "/", itr->second));

         tiles[id].attr() = std::move(attrs);
      }
   }

   void Tilemap::add_layer(std::map<unsigned, Surface>& tiles, xml_node node,
         int tilewidth, int tileheight)
   {
      Layer layer;
      int width  = node.attribute("width").as_int();
      int height = node.attribute("height").as_int();

      if (!width || !height)
         throw std::logic_error("Layer is empty.");

      std::cerr << "Adding layer:" <<
         " Name: " << node.attribute("name").value() <<
         " Width: " << width <<
         " Height: " << height << std::endl;

      Utils::xml_node_walker walk{node.child("data"), "tile", "gid"};
      int index = 0;
      for (auto& gid_str : walk)
      {
         Pos pos = {index % width, index / width};

         unsigned gid = Utils::string_cast<unsigned>(gid_str);
         if (gid)
         {
            auto& surf = tiles[gid];
            surf.rect().pos = pos * Pos{tilewidth, tileheight};

            layer.cluster.vec().push_back(SurfaceCluster::Elem{surf, Pos{}});

            if (Utils::find_or_default(surf.attr(), "collision", "") == "true")
               collisions.insert(pos);
         }

         index++;
      }

      layer.attr = get_attributes(node.child("properties"), "property");
      layer.name = node.attribute("name").value();
      m_layers.push_back(std::move(layer));
   }

   void Tilemap::pos(Pos position)
   {
      for (auto& layer : m_layers)
         layer.cluster.pos(position);
      Renderable::pos(position);
   }

   void Tilemap::render(RenderTarget& target)
   {
      for (auto& layer : m_layers)
         layer.cluster.render(target);
   }

   bool Tilemap::collision(Pos tile) const
   {
      return collisions.count(tile) ||
         find_tile("blocks", {tile.x * tilewidth, tile.y * tileheight});
   }

   Surface* Tilemap::find_tile(unsigned layer_index, Pos offset)
   {
      auto& layer = m_layers.at(layer_index);
      auto& elems = layer.cluster.vec();

      auto itr = std::find_if(std::begin(elems), std::end(elems), [offset](const SurfaceCluster::Elem& elem) {
               return (elem.surf.rect().pos + elem.offset) == offset;
            });

      if (itr == std::end(elems))
         return nullptr;

      return &itr->surf;
   }

   Surface* Tilemap::find_tile(const std::string& name, Pos pos)
   {
      auto layer = std::find_if(std::begin(m_layers), std::end(m_layers), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer == std::end(m_layers))
         return nullptr;

      return find_tile(std::distance(std::begin(m_layers), layer), pos);
   }

   const Surface* Tilemap::find_tile(unsigned layer_index, Pos offset) const
   {
      auto& layer = m_layers.at(layer_index);
      auto& elems = layer.cluster.vec();

      auto itr = std::find_if(std::begin(elems), std::end(elems), [offset](const SurfaceCluster::Elem& elem) {
               return (elem.surf.rect().pos + elem.offset) == offset;
            });

      if (itr == std::end(elems))
         return nullptr;

      return &itr->surf;
   }

   const Surface* Tilemap::find_tile(const std::string& name, Pos pos) const
   {
      auto layer = std::find_if(std::begin(m_layers), std::end(m_layers), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer == std::end(m_layers))
         return nullptr;

      return find_tile(std::distance(std::begin(m_layers), layer), pos);
   }

   const Tilemap::Layer* Tilemap::find_layer(const std::string& name) const
   {
      auto layer = std::find_if(std::begin(m_layers), std::end(m_layers), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer != std::end(m_layers))
         return &*layer;
      else
         return nullptr;
   }

   Tilemap::Layer* Tilemap::find_layer(const std::string& name)
   {
      auto layer = std::find_if(std::begin(m_layers), std::end(m_layers), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer != std::end(m_layers))
         return &*layer;
      else
         return nullptr;
   }
}

