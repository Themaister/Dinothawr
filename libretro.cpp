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
using namespace Icy;
using namespace std;

static unique_ptr<GameManager> game;
static string game_path;
static string game_path_dir;

static Audio::Mixer mixer;
static SFXManager sfx;
static BGManager bg_music;

static bool use_audio_cb;
static bool use_frame_time_cb;

static retro_usec_t frame_time;
static retro_usec_t time_reference;
static bool present_frame;

namespace Icy
{
   Audio::Mixer& get_mixer() { return mixer; }
   const string& get_basedir() { return game_path_dir; }
   SFXManager& get_sfx() { return sfx; }
   BGManager& get_bg() { return bg_music; }
}

#define AUDIO_FRAMES (44100 / 60)
static int16_t audio_buffer[2 * AUDIO_FRAMES];

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
   
   unsigned width = Game::fb_width, height = Game::fb_height;
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

static void audio_callback(void)
{
   mixer.render(audio_buffer, AUDIO_FRAMES);
   for (unsigned i = 0; i < AUDIO_FRAMES; )
   {
      unsigned written = audio_batch_cb(audio_buffer + 2 * i, AUDIO_FRAMES - i);
      i += written;
   }
}

static void frame_time_cb(retro_usec_t usec)
{
   frame_time += usec;
}

void retro_run(void)
{
   if (!use_frame_time_cb)
      frame_time += time_reference;

   input_poll_cb();

   int frames = (frame_time + (time_reference >> 1)) / time_reference;
   if (frames <= 0)
      video_cb(nullptr, Game::fb_width, Game::fb_height, 0);
   else
   {
      present_frame = false;
      for (int i = 0; i < frames - 1; i++)
         game->iterate();
      present_frame = true;
      game->iterate();
      frame_time -= time_reference * frames;
   }

   get_bg().step(mixer);

   if (!use_audio_cb)
      audio_callback();

   if (game->done())
      environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
}

static void load_game(const string& path)
{
   auto input_cb = [&](Input input) -> bool {
      unsigned btn;
      switch (input)
      {
         case Input::Up:    btn = RETRO_DEVICE_ID_JOYPAD_UP; break;
         case Input::Down:  btn = RETRO_DEVICE_ID_JOYPAD_DOWN; break;
         case Input::Left:  btn = RETRO_DEVICE_ID_JOYPAD_LEFT; break;
         case Input::Right: btn = RETRO_DEVICE_ID_JOYPAD_RIGHT; break;
         case Input::Push:  btn = RETRO_DEVICE_ID_JOYPAD_B; break;
         case Input::Menu:  btn = RETRO_DEVICE_ID_JOYPAD_A; break;
         case Input::Reset: btn = RETRO_DEVICE_ID_JOYPAD_SELECT; break;
         default: return false;
      }

      return input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, btn);
   };

   game = make_unique<GameManager>(path, input_cb,
         [&](const void* data, unsigned width, unsigned height, size_t pitch) {
            if (present_frame)
               video_cb(data, width, height, pitch);
         }
   );
}

void retro_reset(void)
{
   size_t memory_size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
   vector<uint8_t> data(memory_size);
   uint8_t *game_data = reinterpret_cast<uint8_t*>(retro_get_memory_data(RETRO_MEMORY_SAVE_RAM));
   copy(game_data, game_data + memory_size, data.data());
   load_game(game_path);
   game_data = reinterpret_cast<uint8_t*>(retro_get_memory_data(RETRO_MEMORY_SAVE_RAM));
   copy(data.begin(), data.end(), game_data);
}

bool retro_load_game(const struct retro_game_info* info)
{
   try
   {
      struct retro_audio_callback cb = { audio_callback };
      use_audio_cb = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &cb);

      time_reference = 1000000 / 60;
      present_frame = false;
      struct retro_frame_time_callback frame_cb = { frame_time_cb, time_reference };
      use_frame_time_cb = environ_cb(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_cb);

      game_path = info->path;
      game_path_dir = basedir(game_path);
      load_game(game_path);
      mixer = Audio::Mixer();

      retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
      environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);

      return true;
   }
   catch (const exception& e)
   {
      cerr << e.what() << endl;
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

