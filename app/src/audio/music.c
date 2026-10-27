#include <kernel.h>
#include <audio_track.h>
#include <audio_hal.h>
#include <board.h>
#include <sys/atomic.h>
#include <errno.h>
#include <acts_ringbuf.h>
#include "badapple_codec.h"
#include "badapple_music.h"
extern const uint8_t badapple_mp3[];
extern const uint32_t badapple_mp3_bytes;
extern volatile bool bringup_transfer_active,pro2_sound_active;
extern int pro2_audio_volume(void);
volatile bool pro2_music_active;
volatile uint32_t pro2_music_samples,pro2_music_total,pro2_music_loops;
volatile uint32_t pro2_music_written,pro2_music_decode_us,pro2_music_underruns;
volatile int32_t pro2_music_result=-EAGAIN;
static atomic_t wanted=1,restart_request;
/* One second of mono PCM. Keep half a second ready before starting DMA. */
static uint8_t __aligned(4) music_pcm_queue[32768];
static struct acts_ringbuf music_ring;
volatile uint32_t pro2_music_buffer_ms,pro2_music_prefills;
static volatile bool music_video_ready;
static struct k_thread music_thread;
K_THREAD_STACK_DEFINE(music_stack,49152);
void badapple_music_pause(bool pause) { atomic_set(&wanted,!pause); }
void badapple_music_restart(void) { atomic_set(&restart_request,1);atomic_set(&wanted,1); }
uint32_t badapple_music_clock(void) { return pro2_music_samples; }
bool badapple_music_video_ready(void) { return music_video_ready; }
static void close_track(struct audio_track_t *track)
{
    if (track) { audio_track_stop(track);audio_track_destory(track); }
    music_video_ready=false;pro2_music_buffer_ms=0;
    hal_aout_close_pa();board_extern_pa_ctl(1,false);pro2_music_active=false;
}
static void music_main(void *a,void *b,void *c)
{
    struct audio_track_t *track=NULL;
    uint32_t base=0,queued=0;
    bool ended=false,started_playback=false;
    int applied=-1;
    int16_t pcm[576];
    k_msleep(500);
    int rc=badapple_codec_open(badapple_mp3,badapple_mp3_bytes);
    if (rc) {pro2_music_result=rc;return;}
    pro2_music_total=badapple_codec_samples();
    pro2_music_result=0;
    for (;;) {
        bool restart=atomic_set(&restart_request,0)!=0;
        if (track) {
            uint32_t count=hal_aout_channel_get_sample_cnt(track->audio_handle);
            if (count>queued) count=queued;
            pro2_music_samples=MIN(base+count,pro2_music_total);
        }
        if (restart || !atomic_get(&wanted) || bringup_transfer_active || pro2_sound_active) {
            if (track) {base=pro2_music_samples;close_track(track);track=NULL;}
            if (restart) {base=0;pro2_music_samples=0;}
            ended=false;
            k_msleep(5);
            if (!atomic_get(&wanted) || bringup_transfer_active || pro2_sound_active) continue;
        }
        if (!track) {
            rc=badapple_codec_seek(base);
            if (rc) goto failure;
            rc=hal_aout_open_pa();
            if (rc) goto failure;
            applied=pro2_audio_volume();
            audio_system_set_stream_volume(AUDIO_STREAM_LOCAL_MUSIC,applied);
            acts_ringbuf_init(&music_ring,music_pcm_queue,sizeof(music_pcm_queue));
            track=audio_track_create(AUDIO_STREAM_LOCAL_MUSIC,16,AUDIO_FORMAT_PCM_16_BIT,AUDIO_MODE_MONO,&music_ring,NULL,NULL);
            if (!track) {rc=-ENOMEM;goto failure;}
            audio_track_set_waitto_start(track,true);
            audio_track_set_volume(track,applied);audio_track_set_mute(track,applied==0);
            rc=hal_aout_channel_set_pa_vol_level(track->audio_handle,audio_policy_get_pa_volume(AUDIO_STREAM_LOCAL_MUSIC,applied));
            if(!rc) rc=hal_aout_channel_mute_ctl(track->audio_handle,applied==0);
            if(!rc) rc=hal_aout_channel_enable_sample_cnt(track->audio_handle,true);
            if(!rc) rc=hal_aout_channel_reset_sample_cnt(track->audio_handle);
            if(rc) goto failure;
            queued=0;ended=false;started_playback=false;pro2_music_active=true;
        }
        int level=pro2_audio_volume();
        if (level!=applied) {audio_track_set_volume(track,level);audio_track_set_mute(track,level==0);applied=level;}
        io_stream_t stream=audio_track_get_stream(track);
        if (!ended && stream_get_space(stream)>=sizeof(pcm)) {
            uint32_t started=k_cycle_get_32();
            size_t n=badapple_codec_read(pcm,576);
            uint32_t elapsed=k_cyc_to_us_floor32(k_cycle_get_32()-started);
            if (elapsed>pro2_music_decode_us) pro2_music_decode_us=elapsed;
            if (n) {
                int bytes=audio_track_write(track,(uint8_t *)pcm,n*sizeof(pcm[0]));
                if (bytes!=(int)(n*sizeof(pcm[0]))) {rc=-EIO;goto failure;}
                queued+=n;pro2_music_written+=n;
            } else ended=true;
        }
        uint32_t buffered=stream_get_length(stream);
        pro2_music_buffer_ms=buffered/32;
        if(!started_playback && (buffered>=16384 || ended)) {
            audio_track_start(track);started_playback=true;pro2_music_prefills++;
        }
        /* If audio falls behind, skip UI work until PCM has recovered. */
        music_video_ready=started_playback && (buffered>=5120 || base+queued>=pro2_music_total);
        if (track->fill_cnt) pro2_music_underruns=track->fill_cnt;
        if (ended && pro2_music_samples>=pro2_music_total) {
            close_track(track);track=NULL;base=0;pro2_music_samples=0;pro2_music_loops++;ended=false;
        }
        k_msleep(2);
    }
failure:
    close_track(track);pro2_music_result=rc;badapple_codec_close();
}
void badapple_music_init(void)
{
    static bool initialized;
    if (initialized) return;
    initialized=true;
    k_thread_create(&music_thread,music_stack,K_THREAD_STACK_SIZEOF(music_stack),music_main,NULL,NULL,NULL,2,0,K_NO_WAIT);
}
