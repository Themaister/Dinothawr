#include "utils.h"

#if __SSE2__
#include <emmintrin.h>
#elif __ALTIVEC__
#include <altivec.h>
#endif

void audio_mix_volume_C(float *out, const float *in, float vol, size_t samples)
{
   size_t i;
   for (i = 0; i < samples; i++)
      out[i] += in[i] * vol;
}

#if __SSE2__
void audio_mix_volume_SSE2(float *out, const float *in, float vol, size_t samples)
{
   size_t i;
   __m128 volume = _mm_set1_ps(vol);

   for (i = 0; i + 16 <= samples; i += 16, out += 16, in += 16)
   {
      unsigned j;
      __m128 input[4] = {
         _mm_loadu_ps(out +  0),
         _mm_loadu_ps(out +  4),
         _mm_loadu_ps(out +  8),
         _mm_loadu_ps(out + 12),
      };

      __m128 additive[4] = {
         _mm_mul_ps(volume, _mm_loadu_ps(in +  0)),
         _mm_mul_ps(volume, _mm_loadu_ps(in +  4)),
         _mm_mul_ps(volume, _mm_loadu_ps(in +  8)),
         _mm_mul_ps(volume, _mm_loadu_ps(in + 12)),
      };

      for (j = 0; j < 4; j++)
         _mm_storeu_ps(out + 4 * j, _mm_add_ps(input[j], additive[j]));
   }

   audio_mix_volume_C(out, in, vol, samples - i);
}
#endif

