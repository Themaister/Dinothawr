#include "mixer.hpp"
#include "utils.h"
#include <algorithm>

namespace Audio
{
   Mixer::Mixer() : master_vol(1.0f) {}

   void Mixer::add_stream(std::unique_ptr<Stream> str)
   {
      streams.push_back(std::move(str));
   }

   void Mixer::purge_dead_streams()
   {
      streams.erase(std::remove_if(std::begin(streams), std::end(streams),
               [](const std::unique_ptr<Stream> &str) {
                  return !str->valid();
               }), std::end(streams));
   }

   void Mixer::render(float* out_buffer, std::size_t frames)
   {
      purge_dead_streams();

      std::fill(out_buffer, out_buffer + frames * channels, 0.0f);

      buffer.reserve(frames * channels);
      for (auto& stream : streams)
      {
         auto rendered = stream->render(buffer.data(), frames);
         audio_mix_volume(out_buffer, buffer.data(), master_vol * stream->volume(), rendered * channels);
      }
   }

   void Mixer::render(std::int16_t* out_buffer, std::size_t frames)
   {
      conv_buffer.reserve(frames * channels);
      render(conv_buffer.data(), frames);

      audio_convert_float_to_s16(out_buffer, conv_buffer.data(), frames * channels);
   }
}

