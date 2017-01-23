#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <stdint.h>
#include <stddef.h>

#define AUDIO_ALIGNED __attribute__((aligned(16)))

#if __SSE2__
#define audio_mix_volume           audio_mix_volume_SSE2

void audio_mix_volume_SSE2(float *out,
      const float *in, float vol, size_t samples);
#else
#define audio_mix_volume           audio_mix_volume_C
#endif

void audio_mix_volume_C(float *dst, const float *src, float vol, size_t samples);

#endif

