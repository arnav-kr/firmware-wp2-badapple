#ifndef BADAPPLE_MUSIC_H
#define BADAPPLE_MUSIC_H
#include <stdint.h>
#include <stdbool.h>
void badapple_music_init(void);
bool badapple_music_video_ready(void);
extern volatile uint32_t pro2_music_buffer_ms,pro2_music_prefills;
void badapple_music_pause(bool pause);
void badapple_music_restart(void);
uint32_t badapple_music_clock(void);
extern volatile bool pro2_music_active;
extern volatile uint32_t pro2_music_samples,pro2_music_total,pro2_music_loops;
extern volatile uint32_t pro2_music_written,pro2_music_decode_us,pro2_music_underruns;
extern volatile int32_t pro2_music_result;
#endif
