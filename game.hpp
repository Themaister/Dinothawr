#ifndef GAME_HPP__
#define GAME_HPP__

#include "surface.hpp"
#include "tilemap.hpp"

#include <string>
#include <functional>
#include <cstddef>

namespace Icy
{
   enum class Input : unsigned
   {
      Up,
      Down,
      Left,
      Right,
      Push
   };

   class Game
   {
      public:
         Game(const std::string& level);

         void input_cb(std::function<bool (Input)> cb) { m_input_cb = cb; }
         void video_cb(std::function<void (const void*, unsigned, unsigned, std::size_t)> cb) { m_video_cb = cb; }

         int width() const { return map.pix_width(); }
         int height() const { return map.pix_height(); }

         void iterate();
         bool won() const;

      private:
         Blit::Tilemap map;
         Blit::RenderTarget target;
         Blit::Surface player;
         Blit::SurfaceCache cache;
         Input facing;

         std::function<bool (Input)> m_input_cb;
         std::function<void (const void*, unsigned, unsigned, std::size_t)> m_video_cb;

         std::function<bool ()> stepper;
         void run_stepper();

         void update_player();
         void update_input();
         void move_if_no_collision(Input input);
         void push_block();
         bool is_offset_collision(Blit::Surface& surf, Blit::Pos offset);

         bool tile_stepper(Blit::Surface& surf, Blit::Pos step_dir);

         static Blit::Pos input_to_offset(Input input);
         std::string input_to_string(Input input);

         std::vector<Blit::SurfaceCluster::Elem> get_tiles_with_attr(const std::string& layer,
               const std::string& attr, const std::string& val) const;
   };
}

#endif

