#ifndef WP2_VIDEO_PLAYER_H
#define WP2_VIDEO_PLAYER_H
#include <stdbool.h>
#include <stdint.h>

#define VIDEO_WIDTH 192
#define VIDEO_HEIGHT 144
#define VIDEO_PACKED (VIDEO_WIDTH * VIDEO_HEIGHT / 8)

/* UI-thread-owned RGB565 image; video_frame is the next frame to decode. */
extern uint16_t video_pixels[VIDEO_WIDTH * VIDEO_HEIGHT];
extern uint32_t video_frame;
extern const uint32_t badapple_frame_count;
void video_reset(void);
bool video_decode_to(uint32_t target);
#endif
