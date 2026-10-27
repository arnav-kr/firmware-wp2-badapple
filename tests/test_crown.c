#include <assert.h>
#include <stdio.h>
#include "../app/src/input/crown_debounce.h"
#include "../app/src/audio/pro2_tone_pcm.h"
int main(void)
{
    struct pro2_crown_state s={0};
    assert(!pro2_crown_sample(&s,false,0));
    assert(!pro2_crown_sample(&s,true,10));
    assert(!pro2_crown_sample(&s,false,20)); /* contact bounce */
    assert(!pro2_crown_sample(&s,true,30));
    assert(!pro2_crown_sample(&s,true,49));
    assert(pro2_crown_sample(&s,true,50));
    assert(!pro2_crown_sample(&s,true,100));
    assert(!pro2_crown_sample(&s,true,6000)); /* no repeated toggles while held */
    assert(!pro2_crown_sample(&s,false,6010));
    assert(!pro2_crown_sample(&s,false,6030));
    assert(!pro2_crown_sample(&s,true,6040));
    assert(pro2_crown_sample(&s,true,6060));
    struct pro2_crown_state boot={0};
    assert(!pro2_crown_sample(&boot,true,0));
    assert(!pro2_crown_sample(&boot,true,5000)); /* held at boot is ignored */
    assert(!pro2_crown_sample(&boot,false,5010));
    assert(!pro2_crown_sample(&boot,false,5030));
    assert(!pro2_crown_sample(&boot,true,5040));
    assert(pro2_crown_sample(&boot,true,5060));
    struct pro2_crown_state wrap={0};
    assert(!pro2_crown_sample(&wrap,false,UINT32_MAX-30));
    assert(!pro2_crown_sample(&wrap,true,UINT32_MAX-10));
    assert(pro2_crown_sample(&wrap,true,10)); /* tick rollover */
    puts("Crown debounce: bounce, hold, release/repress, boot-held and tick rollover passed.");
    unsigned crossings=0;
    int prev=0;
    int64_t dc=0;
    uint64_t energy=0;
    assert(PRO2_TONE_RATE==PRO2_TONE_SAMPLES);
    for (unsigned i=0;i<PRO2_TONE_SAMPLES;i++) {
        int v=pro2_tone_sample(i);
        assert(v>=-8192 && v<=8192);
        if (prev<=0 && v>0) crossings++;
        energy+=(uint64_t)((int64_t)v*v);
        dc+=v;
        prev=v;
    }
    assert(crossings==500);
    assert(dc>=-1000000 && dc<=1000000);
    assert(energy>(uint64_t)PRO2_TONE_SAMPLES*4000*4000);
    assert(energy<(uint64_t)PRO2_TONE_SAMPLES*6000*6000);
    assert(pro2_tone_sample(0)==0 && pro2_tone_sample(PRO2_TONE_SAMPLES-1)==0);
    assert(pro2_tone_sample(PRO2_TONE_SAMPLES)==0);
    puts("Tone PCM: 1 second, 500Hz, amplitude bounds, DC bound and click-reducing fade passed.");
    return 0;
}
