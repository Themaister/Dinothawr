#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <iterator>

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
   }
}

#endif

