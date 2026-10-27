#ifndef PRO2_TONE_PCM_H
#define PRO2_TONE_PCM_H
#include <stdint.h>
#define PRO2_TONE_RATE 16000u
#define PRO2_TONE_SAMPLES 16000u
static const int16_t pro2_sine32[32] = {
    0,1598,3134,4552,5792,6812,7568,8034,8192,8034,7568,6812,5792,4552,3134,1598,
    0,-1598,-3134,-4552,-5792,-6812,-7568,-8034,-8192,-8034,-7568,-6812,-5792,-4552,-3134,-1598
};
static inline int16_t pro2_tone_sample(unsigned pos)
{
    if (pos>=PRO2_TONE_SAMPLES) return 0;
    unsigned ramp=pos;
    if (PRO2_TONE_SAMPLES-1-pos<ramp) ramp=PRO2_TONE_SAMPLES-1-pos;
    if (ramp>160) ramp=160;
    return (int16_t)((int32_t)pro2_sine32[pos%32]*(int32_t)ramp/160);
}
#endif
