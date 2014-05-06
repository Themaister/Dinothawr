#ifndef SURFACE_HPP__
#define SURFACE_HPP__

#include "blit.hpp"

#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <utility>

namespace Blit
{
   class Surface
   {
      public:
         struct Data
         {
            Data(std::vector<Pixel> pixels, int w, int h);
            Data(Pixel pixel, int w, int h);

            std::vector<Pixel> pixels;
            int w, h;
         };

         struct Alt
         {
            std::shared_ptr<const Data> data;
            std::string tag; 
         };

         Surface();
         Surface(Pixel pix, int width, int height);
         Surface(std::shared_ptr<const Data> data);
         Surface(const std::vector<Alt>& alts, const std::string& start_id);

         Surface sub(Rect rect) const;
         void refill_color(Pixel pix);

         Rect& rect() { return m_rect; }
         const Rect& rect() const { return m_rect; }

         void ignore_camera(bool ignore);
         bool ignore_camera() const;

         Pixel pixel(Pos pos) const;
         const Pixel* pixel_raw(Pos pos) const;

         std::pair<std::string, unsigned> active_alt() const { return std::pair<std::string, unsigned>(m_active_alt, m_active_alt_index); }
         void active_alt(const std::string& id, unsigned index = 0);
         void active_alt_index(unsigned index);

         std::map<std::string, std::string>& attr() { return attribs; }
         const std::map<std::string, std::string>& attr() const { return attribs; }

      private:
         std::shared_ptr<const Data> data;

         std::multimap<std::string, std::shared_ptr<const Data>> alts;
         std::string m_active_alt;
         unsigned m_active_alt_index;

         std::map<std::string, std::string> attribs;
         Rect m_rect;
         bool m_ignore_camera;
   };

   class RenderTarget;

   class Renderable
   {
      public:
         virtual void render(RenderTarget& target) const = 0;
         virtual Pos pos() const { return position; }
         virtual void pos(Pos position) { this->position = position; }

         void move(Pos offset) { pos(pos() + offset); }

      protected:
         Pos position;
   };

   class SurfaceCluster : public Renderable
   {
      public:
         struct Elem
         {
            Surface surf;
            Pos offset;
            unsigned tag;
         };

         SurfaceCluster()
         {
         }

         std::vector<Elem>& vec();
         const std::vector<Elem>& vec() const;

         void set_transform(std::function<Pos (Pos)> func);
         void render(RenderTarget& target) const;

      private:
         std::vector<Elem> elems;
         std::function<Pos (Pos)> func;
   };

   class SurfaceCache
   {
      public:
         Surface from_image(const std::string& path);
         Surface from_sprite(const std::string& path);

      private:
         std::map<std::string, std::shared_ptr<const Surface::Data>> cache;
         std::shared_ptr<const Surface::Data> load_image(const std::string& path);
   };

   class RenderTarget
   {
      public:
         RenderTarget()
         {
         }

         RenderTarget(int width, int height);

         Surface convert_surface();

         const Pixel* buffer() const;
         Pixel* pixel_raw(Pos pos);
         Pixel* pixel_raw_no_offset(Pos pos);

         int width() const;
         int height() const;

         void clear(Pixel pix);

         void camera_move(Pos pos);
         void camera_set(Pos pos);
         Pos camera_pos() const;

         void blit(const Surface& surf, Rect subrect);
         void blit_offset(const Surface& surf, Rect subrect, Pos offset);

      private:
         std::vector<Pixel> m_buffer;
         Rect rect;
   };
}

#endif

