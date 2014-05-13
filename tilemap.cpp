#include "tilemap.hpp"
#include "utils.hpp"

#include <iostream>
#include <stdexcept>
#include <map>
#include <utility>
#include <string>
#include "pugixml/pugixml.hpp"

using namespace pugi;

namespace Blit
{
   Tilemap::Tilemap(const std::string& path) : dir(Utils::basedir(path))
   {
      xml_document doc;
      if (!doc.load_file(path.c_str()))
         throw std::runtime_error(Utils::join("Failed to load XML map: ", path, "."));

      xml_node map   = doc.child("map");
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
         const char *name = node.attribute("name").value();
         const char *value = node.attribute("value").value();
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

      pugi::xml_node image     = node.child("image");
      const char *source    = image.attribute("source").value();
      int width      = image.attribute("width").as_int();
      int height     = image.attribute("height").as_int();

#if 0
      std::cerr << "Adding tileset:" <<
         " Name: " << node.attribute("name").value() <<
         " Gid: " << first_gid <<
         " Tilewidth: " << tilewidth <<
         " Tileheight: " << tileheight <<
         " Source: " << source <<
         " Width: " << width <<
         " Height: " << height << std::endl;
#endif

      if (!width || !height || !tilewidth || !tileheight)
         throw std::logic_error("Tilemap is malformed.");

      SurfaceCache cache;
      Blit::Surface surf = cache.from_image(Utils::join(dir, "/", source));

      if (surf.rect().w != width || surf.rect().h != height)
         throw std::logic_error("Tilemap geometry does not correspond with image values.");

      std::map<std::basic_string<char>, std::basic_string<char> > global_attr = get_attributes(node.child("properties"), "property");

#if 0
      std::cerr << "Dumping attrs:" << std::endl;
      for (auto& attr : global_attr)
         std::cerr << "Found global attr (" << attr.first << " => " << attr.second << ")." << std::endl;
      std::cerr << "Dumped attrs." << std::endl;
#endif

      for (int y = 0; y < height; y += tileheight)
      {
         for (int x = 0; x < width; x += tilewidth, id_cnt++)
         {
            int id = first_gid + id_cnt;
            tiles[id] = surf.sub({{x, y}, tilewidth, tileheight});
            std::copy(global_attr.begin(), global_attr.end(), std::inserter(tiles[id].attr(), tiles[id].attr().begin())); 
         }
      }

      // Load all attributes for a tile into the surface.
      for (auto tile = node.child("tile"); tile; tile = tile.next_sibling("tile"))
      {
         int id = first_gid + tile.attribute("id").as_int();

         std::map<std::basic_string<char>, std::basic_string<char> > attrs = get_attributes(tile.child("properties"), "property");
         std::copy(global_attr.begin(), global_attr.end(), std::inserter(attrs, attrs.begin()));

         auto itr = attrs.find("sprite");

         if (itr != attrs.end())
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

#if 0
      std::cerr << "Adding layer:" <<
         " Name: " << node.attribute("name").value() <<
         " Width: " << width <<
         " Height: " << height << std::endl;
#endif

      Utils::xml_node_walker walk{node.child("data"), "tile", "gid"};
      int index = 0;
      for (auto& gid_str : walk)
      {
         Pos pos = Pos(index % width, index / width);

         unsigned gid = Utils::stoi(gid_str);
         if (gid)
         {
            Blit::Surface surf = tiles[gid];
            surf.rect().pos = pos * Pos(tilewidth, tileheight);

            layer.cluster.vec().push_back({surf, Pos()});

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

   void Tilemap::render(RenderTarget& target) const
   {
      for (auto& layer : m_layers)
         layer.cluster.render(target);
   }

   void Tilemap::render_until_layer(unsigned index, RenderTarget& target) const
   {
      for (unsigned i = 0; i <= index; i++)
         m_layers.at(i).cluster.render(target);
   }

   void Tilemap::render_after_layer(unsigned index, RenderTarget& target) const
   {
      for (unsigned i = index + 1; i < m_layers.size(); i++)
         m_layers.at(i).cluster.render(target);
   }

   bool Tilemap::collision(Pos tile) const
   {
      return collisions.count(tile) ||
         find_tile("blocks", {tile.x * tilewidth, tile.y * tileheight});
   }

   Surface* Tilemap::find_tile(unsigned layer_index, Pos offset)
   {
      Blit::Tilemap::Layer& layer = m_layers.at(layer_index);
      std::vector<Blit::SurfaceCluster::Elem>& elems = layer.cluster.vec();

      std::vector<Blit::SurfaceCluster::Elem>::iterator itr = std::find_if(elems.begin(), elems.end(), [offset](const SurfaceCluster::Elem& elem) {
               return (elem.surf.rect().pos + elem.offset) == offset;
            });

      if (itr == elems.end())
         return NULL;

      return &itr->surf;
   }

   Surface* Tilemap::find_tile(const std::string& name, Pos pos)
   {
      std::vector<Blit::Tilemap::Layer>::iterator layer = std::find_if(m_layers.begin(), m_layers.end(), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer == m_layers.end())
         return NULL;

      return find_tile(std::distance(m_layers.begin(), layer), pos);
   }

   const Surface* Tilemap::find_tile(unsigned layer_index, Pos offset) const
   {
      const Blit::Tilemap::Layer& layer = m_layers.at(layer_index);
      const std::vector<Blit::SurfaceCluster::Elem>& elems = layer.cluster.vec();

      auto itr = std::find_if(elems.begin(), elems.end(), [offset](const SurfaceCluster::Elem& elem) {
               return (elem.surf.rect().pos + elem.offset) == offset;
            });

      if (itr == elems.end())
         return NULL;

      return &itr->surf;
   }

   const Surface* Tilemap::find_tile(const std::string& name, Pos pos) const
   {
      auto layer = std::find_if(m_layers.begin(), m_layers.end(), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer == m_layers.end())
         return NULL;

      return find_tile(std::distance(m_layers.begin(), layer), pos);
   }

   const Tilemap::Layer* Tilemap::find_layer(const std::string& name) const
   {
      auto layer = std::find_if(m_layers.begin(), m_layers.end(), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer != m_layers.end())
         return &*layer;
      else
         return NULL;
   }

   int Tilemap::find_layer_index(const std::string& name) const
   {
      auto layer = std::find_if(m_layers.begin(), m_layers.end(), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer != m_layers.end())
         return layer - m_layers.begin();
      else
         return -1;
   }

   Tilemap::Layer* Tilemap::find_layer(const std::string& name)
   {
      auto layer = std::find_if(m_layers.begin(), m_layers.end(), [&name](const Layer& layer) {
               return Utils::tolower(layer.name) == name;
            });

      if (layer != m_layers.end())
         return &*layer;
      else
         return NULL;
   }
}

