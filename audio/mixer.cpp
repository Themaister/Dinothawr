#include "mixer.hpp"
#include "utils.h"
#include "../utils.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace Blit::Utils;

namespace Audio
{
   Mixer::Mixer() : master_vol(1.0f) {}

   void Mixer::add_stream(std::shared_ptr<Stream> str)
   {
      streams.push_back(std::move(str));
   }

   void Mixer::purge_dead_streams()
   {
      streams.erase(std::remove_if(std::begin(streams), std::end(streams),
               [](const std::shared_ptr<Stream> &str) {
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


   VorbisFile::VorbisFile(const std::string& path)
      : is_eof(false)
   {
      if (ov_fopen(path.c_str(), &vf) < 0)
         throw std::runtime_error(join("Failed to open vorbis file: ", path));

      loop(true);

      std::cerr << "Vorbis info:" << std::endl;
      std::cerr << "\tStreams: " << ov_streams(&vf) << std::endl;

      auto info = ov_info(&vf, 0);
      if (info)
      {
         if (info->channels != static_cast<int>(Mixer::channels))
            throw std::logic_error(join("Vorbis file has ", info->channels, " channels."));

         if (info->rate != 44100)
            throw std::logic_error(join("Sampling rate of file is: ", info->rate));
      }
      else
         throw std::logic_error("Couldn't find info for vorbis file.");
   }

   VorbisFile::~VorbisFile()
   {
      ov_clear(&vf);
   }

   std::size_t VorbisFile::render(float* buffer, std::size_t frames)
   {
      std::size_t rendered = 0;

      while (frames)
      {
         float **pcm;
         int bitstream;
         long ret = ov_read_float(&vf, &pcm, frames, &bitstream);

         if (ret < 0)
            throw std::runtime_error(join("Vorbis decoding failed with: ", ret));

         if (ret == 0) // EOF
         {
            if (loop())
            {
               loop(false); // Avoid infinite recursion incause our audio clip is really short.
               ScopeExit holder([this] { loop(true); });

               if (ov_time_seek(&vf, 0.0) == 0)
               {
                  auto ret = render(buffer, frames);
                  return rendered + ret;
               }
               else
                  is_eof = true;
            }
            else
               is_eof = true;

            return rendered;
         }

         for (unsigned c = 0; c < Mixer::channels; c++)
            for (long i = 0; i < ret; i++)
               buffer[2 * i + c] = pcm[c][i];

         buffer += ret * Mixer::channels;
         frames -= ret;
         rendered += ret;
      }

      return rendered;
   }
}

