#include "libretro.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>

#include "game.hpp"
#include "utils.hpp"
#include "audio/mixer.hpp"

using namespace Blit::Utils;

static std::unique_ptr<Icy::GameManager> game;
static std::string game_path;
static std::string game_path_dir;

static Audio::Mixer mixer;
static Icy::SFXManager sfx;

namespace Icy
{
   Audio::Mixer& get_mixer() { return mixer; }
   const std::string& get_basedir() { return game_path_dir; }
   SFXManager& get_sfx() { return sfx; }
}

#define AUDIO_FRAMES (44100 / 60)
static std::int16_t audio_buffer[2 * AUDIO_FRAMES];

void retro_init(void)
{}

void retro_deinit(void)
{}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned, unsigned)
{}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Dinothawr";
   info->library_version  = "v0";
   info->need_fullpath    = true;
   info->valid_extensions = "game";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing = { 60.0, 44100.0 };
   
   unsigned width = Icy::Game::fb_width, height = Icy::Game::fb_height;
   info->geometry = { width, height, width, height };
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_run(void)
{
   input_poll_cb();
   game->iterate();

   mixer.render(audio_buffer, AUDIO_FRAMES);

   for (unsigned i = 0; i < AUDIO_FRAMES; )
   {
      unsigned written = audio_batch_cb(audio_buffer + 2 * i, AUDIO_FRAMES - i);
      i += written;
   }

   if (game->done())
      environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
}

static void load_game(const std::string& path)
{
   auto input_cb = [&](Icy::Input input) -> bool {
      unsigned btn;
      switch (input)
      {
         case Icy::Input::Up:    btn = RETRO_DEVICE_ID_JOYPAD_UP; break;
         case Icy::Input::Down:  btn = RETRO_DEVICE_ID_JOYPAD_DOWN; break;
         case Icy::Input::Left:  btn = RETRO_DEVICE_ID_JOYPAD_LEFT; break;
         case Icy::Input::Right: btn = RETRO_DEVICE_ID_JOYPAD_RIGHT; break;
         case Icy::Input::Push:  btn = RETRO_DEVICE_ID_JOYPAD_A; break;
         case Icy::Input::Menu:  btn = RETRO_DEVICE_ID_JOYPAD_B; break;
         case Icy::Input::Reset: btn = RETRO_DEVICE_ID_JOYPAD_SELECT; break;
         default: return false;
      }

      return input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, btn);
   };

   game = make_unique<Icy::GameManager>(path, input_cb,
         [&](const void* data, unsigned width, unsigned height, std::size_t pitch) {
            video_cb(data, width, height, pitch);
         }
   );
}

void retro_reset(void)
{
   size_t memory_size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
   std::vector<uint8_t> data(memory_size);
   uint8_t *game_data = reinterpret_cast<uint8_t*>(retro_get_memory_data(RETRO_MEMORY_SAVE_RAM));
   std::copy(game_data, game_data + memory_size, data.data());
   load_game(game_path);
   game_data = reinterpret_cast<uint8_t*>(retro_get_memory_data(RETRO_MEMORY_SAVE_RAM));
   std::copy(data.begin(), data.end(), game_data);
}

bool retro_load_game(const struct retro_game_info* info)
{
   try
   {
      game_path = info->path;
      game_path_dir = basedir(game_path);

      load_game(game_path);

      mixer = Audio::Mixer();

#if 0
      Audio::VorbisFile bg{join(Icy::get_basedir(), "/assets/bg.ogg")};
      auto stream = std::make_shared<Audio::PCMStream>(std::make_shared<std::vector<float>>(bg.decode())); // Kinda hardcoded atm.
      stream->loop(true);
      mixer.add_stream(stream);
#endif

      retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
      environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);

      return true;
   }
   catch(const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
      return false;
   }
}

bool retro_load_game_special(unsigned, const struct retro_game_info*, size_t)
{
   return false;
}

void retro_unload_game(void)
{
   game.reset();
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void*, size_t)
{
   return false;
}

bool retro_unserialize(const void*, size_t)
{
   return false;
}

void* retro_get_memory_data(unsigned id)
{
   if (id != RETRO_MEMORY_SAVE_RAM)
      return nullptr;

   return game->save_data();
}

size_t retro_get_memory_size(unsigned id)
{
   if (id != RETRO_MEMORY_SAVE_RAM)
      return 0;

   return game->save_size();
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned, bool, const char*)
{}

