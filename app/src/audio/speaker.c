/* Explicit, bounded speaker test. No playback during boot or OTA. */
#include <kernel.h>
#include <sys/atomic.h>
#include <audio_track.h>
#include <board.h>
#include <audio_hal.h>
#include <device.h>
#include <drivers/gpio.h>
#include <errno.h>
#include <soc.h>
#include "pro2_tone_pcm.h"
#ifdef PRO2_MEDIA11
#include "badapple_music.h"
#endif

#define PRO2_DAC_BASE 0x4005C000UL

volatile bool pro2_sound_active;
volatile int32_t pro2_sound_result=-EAGAIN;
volatile uint32_t pro2_sound_bytes;
volatile uint32_t pro2_sound_tests;
static atomic_t sound_busy;
static atomic_t volume_level=10;
volatile uint32_t pro2_sound_samples;
volatile int32_t pro2_sound_pa_level,pro2_sound_pa_rc;
extern volatile int32_t pro2_pa_last_rc;
volatile int32_t pro2_sound_prime_rc;
volatile int32_t pro2_sound_gain_rc;
volatile uint32_t pro2_sound_pcm_blocks,pro2_sound_pcm_nonzero;
volatile int32_t pro2_sound_pcm_min,pro2_sound_pcm_max;
volatile uint32_t pro2_sound_pcm_peak,pro2_sound_pcm_sum;
volatile uint32_t pro2_sound_dac_digctl,pro2_sound_dac_fifoctl,pro2_sound_dac_stat;
volatile uint32_t pro2_sound_dac_pcm_buf_ctl,pro2_sound_dac_pcm_buf_stat,pro2_sound_dac_pcm_buf_cnt;
volatile uint32_t pro2_sound_dac_vol_lch,pro2_sound_dac_vol_rch;
volatile uint32_t pro2_sound_dac_anactl0,pro2_sound_dac_anactl1,pro2_sound_dac_anactl2,pro2_sound_dac_bias;
volatile uint32_t pro2_sound_audio_ldo,pro2_sound_cmu_dacclk;
volatile uint32_t pro2_sound_pa_odat,pro2_sound_rail_odat;
volatile uint32_t pro2_sound_dac_capture_valid;
volatile uint32_t pro2_dma_blocks,pro2_dma_nonzero,pro2_dma_peak,pro2_dma_bytes;
void pro2_pcm_dma_probe(const void *buffer,unsigned bytes) {
    if(!pro2_sound_active || !buffer) return;
    const int16_t *pcm=buffer;uint32_t nz=0,peak=0;
    for(unsigned i=0;i<bytes/2;i++) {
        int v=pcm[i];unsigned magnitude=v<0 ? (unsigned)-v : (unsigned)v;
        if(v) nz++;
        if(magnitude>peak) peak=magnitude;
    }
    pro2_dma_blocks++;pro2_dma_bytes+=bytes;pro2_dma_nonzero+=nz;
    if(peak>pro2_dma_peak) pro2_dma_peak=peak;
}
int pro2_audio_volume(void) {return atomic_get(&volume_level);}
void pro2_audio_volume_steps(int steps) {
    atomic_val_t old,next;
    do {old=atomic_get(&volume_level);next=old+steps;if(next<0)next=0;if(next>15)next=15;}
    while(!atomic_cas(&volume_level,old,next));
}

extern volatile bool bringup_transfer_active;

int pro2_play_test_tone(void)
{
#ifdef PRO2_MEDIA11
    if (pro2_music_active) return -EBUSY;
#endif
    if (bringup_transfer_active || !atomic_cas(&sound_busy,0,1)) return -EBUSY;
    pro2_sound_active=true;
    pro2_sound_bytes=0;
    pro2_sound_tests++;
    pro2_dma_blocks=pro2_dma_nonzero=pro2_dma_peak=pro2_dma_bytes=0;
    pro2_sound_pcm_blocks=0;
    pro2_sound_pcm_nonzero=0;
    pro2_sound_pcm_min=32767;
    pro2_sound_pcm_max=-32768;
    pro2_sound_pcm_peak=0;
    pro2_sound_pcm_sum=0;
    pro2_sound_dac_capture_valid=0;
    int rc=hal_aout_open_pa();
    pro2_sound_prime_rc=rc;
    pro2_sound_samples=0;
    if(rc) goto done;
    rc=-ENOMEM;
    audio_system_set_stream_volume(AUDIO_STREAM_LOCAL_MUSIC,pro2_audio_volume());
    struct audio_track_t *track=audio_track_create(AUDIO_STREAM_LOCAL_MUSIC,16,
        AUDIO_FORMAT_PCM_16_BIT,AUDIO_MODE_MONO,NULL,NULL,NULL);
    if (!track) goto done;
    audio_track_set_volume(track,pro2_audio_volume());
    audio_track_set_mute(track,pro2_audio_volume()==0);
    rc=hal_aout_channel_set_pa_vol_level(track->audio_handle,audio_policy_get_pa_volume(AUDIO_STREAM_LOCAL_MUSIC,pro2_audio_volume()));
    if(!rc) rc=hal_aout_channel_mute_ctl(track->audio_handle,pro2_audio_volume()==0);
    pro2_sound_gain_rc=rc;
    if(rc) goto close;
    rc=hal_aout_channel_enable_sample_cnt(track->audio_handle,true);
    if(!rc) rc=hal_aout_channel_reset_sample_cnt(track->audio_handle);
    if(rc) goto close;
    io_stream_t stream=audio_track_get_stream(track);
    uint32_t start=k_uptime_get_32();
    int16_t block[256];
    while (pro2_sound_bytes < 32000) {
        if ((uint32_t)(k_uptime_get_32()-start)>2500) { rc=-ETIMEDOUT; goto close; }
        unsigned n=MIN(sizeof(block),32000-pro2_sound_bytes);
        if (stream_get_space(stream)<n) { k_msleep(2); continue; }
        unsigned first=pro2_sound_bytes/2;
        int16_t mn=32767,mx=-32768;
        uint32_t nz=0,peak=0,sum=0;
        for (unsigned i=0;i<n/2;i++) {
            int16_t s=pro2_tone_sample(first+i);
            block[i]=s;
            if(s<mn)mn=s;
            if(s>mx)mx=s;
            if(s)nz++;
            uint32_t a=(s<0)?(uint32_t)(-(int32_t)s):(uint32_t)s;
            if(a>peak)peak=a;
            sum+=a;
        }
        if(mn<pro2_sound_pcm_min) pro2_sound_pcm_min=mn;
        if(mx>pro2_sound_pcm_max) pro2_sound_pcm_max=mx;
        if(peak>pro2_sound_pcm_peak) pro2_sound_pcm_peak=peak;
        pro2_sound_pcm_sum+=sum;
        pro2_sound_pcm_nonzero+=nz;
        pro2_sound_pcm_blocks++;
        int wrote=audio_track_write(track,(unsigned char *)block,n);
        if (wrote<=0) { rc=-EIO; goto close; }
        pro2_sound_bytes+=wrote;
        audio_track_start(track);
    }
    start=k_uptime_get_32();
    int applied_level=pro2_audio_volume();
    while (1) {
        uint32_t samples=hal_aout_channel_get_sample_cnt(track->audio_handle);
        pro2_sound_samples=samples;
        bool drained=stream_get_length(stream)<=0 && samples>=16000;
        if((uint32_t)(k_uptime_get_32()-start)>4000) {rc=-ETIMEDOUT;goto close;}
        if (!pro2_sound_dac_capture_valid && samples>0) {
            pro2_sound_dac_digctl=sys_read32(PRO2_DAC_BASE+0x00);
            pro2_sound_dac_fifoctl=sys_read32(PRO2_DAC_BASE+0x04);
            pro2_sound_dac_stat=sys_read32(PRO2_DAC_BASE+0x08);
            pro2_sound_dac_pcm_buf_ctl=sys_read32(PRO2_DAC_BASE+0x10);
            pro2_sound_dac_pcm_buf_stat=sys_read32(PRO2_DAC_BASE+0x14);
            pro2_sound_dac_vol_lch=sys_read32(PRO2_DAC_BASE+0x28);
            pro2_sound_dac_vol_rch=sys_read32(PRO2_DAC_BASE+0x2C);
            pro2_sound_dac_pcm_buf_cnt=sys_read32(PRO2_DAC_BASE+0x30);
            pro2_sound_dac_anactl0=sys_read32(PRO2_DAC_BASE+0x34);
            pro2_sound_dac_anactl1=sys_read32(PRO2_DAC_BASE+0x38);
            pro2_sound_dac_anactl2=sys_read32(PRO2_DAC_BASE+0x3C);
            pro2_sound_dac_bias=sys_read32(PRO2_DAC_BASE+0x40);
            pro2_sound_audio_ldo=sys_read32(AUDIOLDO_CTL);
            pro2_sound_cmu_dacclk=sys_read32(CMU_DACCLK);
            pro2_sound_pa_odat=sys_read32(GPION_ODAT(21));
            pro2_sound_rail_odat=sys_read32(GPION_ODAT(31));
            pro2_sound_dac_capture_valid=1;
        }
        if(drained) break;
        int level=pro2_audio_volume();
        if(level!=applied_level) {audio_track_set_volume(track,level);audio_track_set_mute(track,level==0);applied_level=level;}
        k_msleep(10);
    }
    k_msleep(80);
    pro2_sound_pa_level=(int32_t)((sys_read32(GPION_ODAT(21))>>21)&1);
    pro2_sound_pa_rc=pro2_pa_last_rc;
    rc=pro2_sound_pa_level==1 && pro2_sound_pa_rc==0 ? 0 : -EIO;
close:
    audio_track_stop(track);
    audio_track_destory(track);
done:
    hal_aout_close_pa();
    board_extern_pa_ctl(1,false);
    pro2_sound_result=rc;
    pro2_sound_active=false;
    atomic_clear(&sound_busy);
    return rc;
}
