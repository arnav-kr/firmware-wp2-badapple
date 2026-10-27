#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "badapple_codec.h"
/* Buffer-backed decoder only allocates its seek index. Bound it explicitly. */
static union { uint64_t alignment; uint8_t bytes[131072]; } index_arena;
static void *codec_realloc(void *old, size_t size)
{
    if (old && old != index_arena.bytes) return NULL;
    return size && size <= sizeof(index_arena.bytes) ? index_arena.bytes : NULL;
}
static void codec_free(void *pointer) { (void)pointer; }
#define realloc codec_realloc
#define free codec_free
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_NO_STDIO
#include "minimp3_ex.h"
#undef realloc
#undef free
static mp3dec_ex_t decoder;
int badapple_codec_open(const uint8_t *data, size_t size)
{
    int rc=mp3dec_ex_open_buf(&decoder,data,size,MP3D_SEEK_TO_SAMPLE);
    if (rc) return rc;
    if (decoder.info.channels!=1 || decoder.info.hz!=16000 || decoder.samples>UINT32_MAX) {
        mp3dec_ex_close(&decoder);
        return -EINVAL;
    }
    return 0;
}
size_t badapple_codec_read(int16_t *pcm,size_t samples) { return mp3dec_ex_read(&decoder,pcm,samples); }
int badapple_codec_seek(uint32_t sample) { return mp3dec_ex_seek(&decoder,sample); }
uint32_t badapple_codec_samples(void) { return (uint32_t)decoder.samples; }
void badapple_codec_close(void) { mp3dec_ex_close(&decoder); }
