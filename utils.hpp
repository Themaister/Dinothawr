#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <stdexcept>
#include <iterator>

#include "pugixml/pugixml.hpp"

namespace Blit
{
   namespace Utils
   {
      template <typename T>
      inline std::string join(T&& t)
      {
         std::ostringstream stream;
         stream << std::forward<T>(t);
         return stream.str();
      }

      template <typename T, typename... U>
      inline std::string join(T&& t, U&&... u)
      {
         std::ostringstream stream;
         stream << std::forward<T>(t) << join(std::forward<U>(u)...);
         return stream.str();
      }

      inline std::string basedir(const std::string& path)
      {
         auto last = path.find_last_of("/\\");
         if (last != std::string::npos)
            return path.substr(0, last);
         else
            return ".";
      }

      inline std::string tolower(const std::string& str)
      {
         std::string tmp;
         std::transform(std::begin(str), std::end(str), std::back_inserter(tmp), [](char c) -> char { return ::tolower(c); });
         return tmp;
      }

      inline std::string toupper(const std::string& str)
      {
         std::string tmp;
         std::transform(std::begin(str), std::end(str), std::back_inserter(tmp), [](char c) -> char { return ::toupper(c); });
         return tmp;
      }

      template <typename T>
      inline T string_cast(const std::string&);

      template <>
      inline unsigned string_cast<unsigned>(const std::string& str)
      {
         char *endptr;
         unsigned ret = std::strtoul(str.c_str(), &endptr, 0);

         if (endptr - str.c_str() != static_cast<std::ptrdiff_t>(str.size()))
            throw std::logic_error(Utils::join(str, " is not a valid unsigned."));

         return ret;
      }

      template <>
      inline int string_cast<int>(const std::string& str)
      {
         char *endptr;
         int ret = std::strtol(str.c_str(), &endptr, 0);

         if (endptr - str.c_str() != static_cast<std::ptrdiff_t>(str.size()))
            throw std::logic_error(Utils::join(str, " is not a valid unsigned."));

         return ret;
      }

      template <typename T>
      inline auto find_or_default(const T& mapper, const typename T::key_type& key, const typename T::mapped_type& def) -> typename T::mapped_type
      {
         auto itr = mapper.find(key);
         if (itr == std::end(mapper))
            return def;

         return itr->second;
      }

      class xml_node_walker
      {
         public:
            xml_node_walker(pugi::xml_node parent, const std::string& child, const std::string& attr)
               : parent(parent), child(child), attr(attr)
            {}

            class iterator
            {
               public:
                  iterator(pugi::xml_node parent, const char* child, const char* attr) :
                     child_name(child), attr(attr), node(parent.child(child)), val(node.attribute(attr).value()) {}

                  iterator() : child_name(nullptr), attr(nullptr) {}

                  const std::string& operator*() const { return val; }
                  const std::string* operator->() const { return &val; }

                  iterator& operator++()
                  {
                     node = node.next_sibling(child_name);
                     val  = node.attribute(attr).value();
                     return *this;
                  }

                  bool operator==(const iterator& itr) const { return node == itr.node; }
                  bool operator!=(const iterator& itr) const { return node != itr.node; }

               private:
                  const char* child_name;
                  const char* attr;
                  pugi::xml_node node;
                  std::string val;
            };

            iterator begin() { return iterator(parent, child.c_str(), attr.c_str()); }
            iterator end() { return iterator(); }

         private:
            pugi::xml_node parent;
            std::string child;
            std::string attr;
      };
   }
}

#endif

