#ifndef MIXER_HPP__
#define MIXER_HPP__

#include <stdint.h>
#include <string.h>
#ifndef USE_CXX03
#include <vector>
#include <memory>
#include <utility>
#include <cmath>
#include <string>
#include <future>
#include <chrono>
#include <queue>
#include <mutex>
#include <vorbis/vorbisfile.h>
#endif

#ifndef M_PI
#  define M_PI (3.1415926536f)
#endif

namespace Audio
{
#ifndef USE_CXX03
   class Stream
   {
      public:
         Stream() : m_volume(1.0), m_loop(false) {}

         virtual std::size_t render(float* buffer, std::size_t frames) = 0;
         virtual bool valid() const = 0;
         virtual ~Stream() {};

         float volume() const { return m_volume; }
         void volume(float vol) { m_volume = vol; }

         bool loop() const { return m_loop; }
         void loop(bool l) { m_loop = l; }

         virtual void rewind() {};

      private:
         float m_volume;
         bool m_loop;
   };

   class SineStream : public Stream
   {
      public:
         SineStream(float freq, float sample_rate) : omega(2.0 * M_PI * freq / sample_rate), index(0.0) {}

         std::size_t render(float* buffer, std::size_t frames)
         {
            std::size_t i;
            for (i = 0; i < frames; i++, index += omega)
            {
               float val         = std::sin(index);
               buffer[2 * i + 0] = buffer[2 * i + 1] = val;
            }

            return frames;
         }

         bool valid() const { return true; }

      private:
         double omega;
         double index;
   };

   class PCMStream : public Stream
   {
      public:
         PCMStream(std::shared_ptr<std::vector<float>> data);

         bool valid() const { return ptr < data->size(); }
         void rewind() { ptr = 0; }
         std::size_t render(float* buffer, std::size_t frames);

      private:
         std::shared_ptr<std::vector<float>> data;
         std::atomic<std::size_t> ptr;
   };

   class WAVFile
   {
      public:
         WAVFile() = delete;
         static std::vector<float> load_wave(const std::string& path);
   };

   class VorbisFile : public Stream
   {
      public:
         VorbisFile(const std::string& path);
         VorbisFile& operator=(const VorbisFile&) = delete;
         VorbisFile(const VorbisFile&) = delete;

         ~VorbisFile();

         std::size_t render(float* buffer, std::size_t frames);
         bool valid() const { return !is_eof; }
         void rewind();
         std::shared_ptr<VorbisFile> dup() const { return std::make_shared<VorbisFile>(path); }
         std::vector<float> decode();

      private:
         std::string path;
         OggVorbis_File vf;
         bool is_eof;
         bool is_mono;
   };

   class Mixer;
   class VorbisLoader
   {
      public:
         void request_vorbis(const std::string& path);
         std::shared_ptr<std::vector<float>> flush();
         size_t size() const { return inflight.size(); }

      private:
         std::vector<std::future<std::vector<float>>> inflight;
         std::queue<std::vector<float>> finished;

         void cleanup();
   };

   class Mixer
   {
      public:
         static const unsigned channels = 2;

         Mixer();

         void add_stream(std::shared_ptr<Stream> str);

         void clear();

         void render(float *buffer, std::size_t frames);
         void render(int16_t *buffer, std::size_t frames);
         void master_volume(float vol) { master_vol = vol; }
         float master_volume() const { return master_vol; }

         void lock() { m_lock->lock(); }
         void unlock() { m_lock->unlock(); }

         void enable(bool enable) { m_enabled->store(enable); }
         bool enabled() const { return m_enabled->load(); }

      private:
         std::vector<float> buffer;
         std::vector<float> conv_buffer;
         std::vector<std::shared_ptr<Stream>> streams;
         std::unique_ptr<std::recursive_mutex> m_lock;
         std::unique_ptr<std::atomic<unsigned>> m_enabled; // Have to use pointer as std::atomic is not CopyAssignable.

         float master_vol;
         void purge_dead_streams();
   };
#else
   class Mixer
   {
      public:
         static const unsigned channels = 2;

         Mixer() {}

         void render(float *buffer, size_t frames) { memset(buffer, 0, sizeof(float)*frames); }
         void render(int16_t *buffer, size_t frames) { memset(buffer, 0, sizeof(int16_t)*frames); }

         void enable(bool enable) { }
         bool enabled() const { return false; }
   };
#endif
}

#endif

