#ifndef BADAPPLE_CODEC_H
#define BADAPPLE_CODEC_H
#include <stdint.h>
#include <stddef.h>
int badapple_codec_open(const uint8_t *data, size_t size);
size_t badapple_codec_read(int16_t *pcm, size_t samples);
int badapple_codec_seek(uint32_t sample);
uint32_t badapple_codec_samples(void);
void badapple_codec_close(void);
#endif
