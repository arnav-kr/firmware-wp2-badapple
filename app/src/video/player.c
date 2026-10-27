#include "player.h"
#include <string.h>
#include "miniz_tinfl.h"

#ifdef _MSC_VER
#define WP2_ALIGNED4 __declspec(align(4))
#else
#define WP2_ALIGNED4 __attribute__((aligned(4)))
#endif

extern const uint8_t badapple_data[];
extern const uint32_t badapple_offsets[];
static tinfl_decompressor decoder;
static uint8_t delta[VIDEO_PACKED];
static uint8_t previous[VIDEO_PACKED];
WP2_ALIGNED4 uint16_t video_pixels[VIDEO_WIDTH * VIDEO_HEIGHT];
uint32_t video_frame;

void video_reset(void)
{
    video_frame = 0;
    memset(previous, 0, sizeof(previous));
}

static bool decode_next(bool render)
{
    if (video_frame >= badapple_frame_count) {
        video_frame = 0;
        memset(previous, 0, sizeof(previous));
    }
    uint32_t entry = badapple_offsets[video_frame];
    uint32_t start = entry & 0x7fffffffu;
    uint32_t end = badapple_offsets[video_frame + 1] & 0x7fffffffu;
    if (end <= start) {
        return false;
    }
    size_t input_size = end - start;
    size_t output_size = sizeof(delta);
    tinfl_init(&decoder);
    tinfl_status status = tinfl_decompress(
        &decoder, badapple_data + start, &input_size, delta, delta, &output_size,
        TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (status != TINFL_STATUS_DONE || output_size != VIDEO_PACKED ||
        input_size != end - start) {
        return false;
    }
    for (size_t i = 0; i < VIDEO_PACKED; ++i) {
        uint8_t value = (entry & 0x80000000u) ? delta[i] : previous[i] ^ delta[i];
        previous[i] = value;
        if (render) {
            for (unsigned bit = 0; bit < 8; ++bit) {
                video_pixels[i * 8 + bit] = (value & (0x80u >> bit)) ? 0xffff : 0;
            }
        }
    }
    ++video_frame;
    return true;
}

bool video_decode_to(uint32_t target)
{
    if (target >= badapple_frame_count) {
        return false;
    }
    if (video_frame == target + 1) {
        return true;
    }
    /* Backward/large seeks restart at a keyframe to reconstruct delta history. */
    if (target < video_frame || target - video_frame > 8) {
        uint32_t key = target;
        while (key && !(badapple_offsets[key] & 0x80000000u)) {
            --key;
        }
        video_frame = key;
        memset(previous, 0, sizeof(previous));
    }
    while (video_frame <= target) {
        if (!decode_next(video_frame == target)) {
            return false;
        }
    }
    return true;
}
