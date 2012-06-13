#include "libretro.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>

#include "surface.hpp"
#include "tilemap.hpp"

static std::unique_ptr<Blit::RenderTarget> target;
static Blit::Tilemap map;

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
   info->library_name     = "libBlit";
   info->library_version  = "v0";
   info->need_fullpath    = true;
   info->valid_extensions = NULL; // Anything is fine, we don't care.
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing = { 60.0, 32000.0 };
   
   unsigned width = map.pix_width(), height = map.pix_height();
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

void retro_reset(void)
{}

static void update_input()
{
   input_poll_cb();

   Blit::Pos camera_dir;
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
      camera_dir += {0, -1};
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
      camera_dir += {-1, 0};
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
      camera_dir += {1, 0};
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
      camera_dir += {0, 1};

   target->camera_move(camera_dir);
}

static unsigned frame_cnt = 0;

static void render_video()
{
   target->clear(Blit::Pixel::ARGB(0xff, 0x80, 0x80, 0x80));
   map.render(*target);

   video_cb(target->buffer(), target->width(), target->height(), target->width() * sizeof(Blit::Pixel));
   
   frame_cnt++;
}

static void render_audio()
{}

void retro_run(void)
{
   update_input();
   render_video();
   render_audio();
}

bool retro_load_game(const struct retro_game_info* info)
{
   try
   {
      map = {info->path};
      target = std::unique_ptr<Blit::RenderTarget>(new Blit::RenderTarget(map.pix_width(), map.pix_height()));
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
{}

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

void* retro_get_memory_data(unsigned)
{
   return nullptr;
}

size_t retro_get_memory_size(unsigned)
{
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned, bool, const char*)
{}

