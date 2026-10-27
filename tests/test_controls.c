#include <assert.h>
#include <stdio.h>
#include "../app/src/input/media_controls.h"
int main(void) {
struct pro2_media_button s={0};
assert(!pro2_media_button_sample(&s,false,0));
assert(!pro2_media_button_sample(&s,true,10));
assert(!pro2_media_button_sample(&s,true,30));
assert(!pro2_media_button_sample(&s,false,100));
assert(pro2_media_button_sample(&s,false,120)==PRO2_ACTION_PLAY_PAUSE);
assert(!pro2_media_button_sample(&s,true,200));
assert(!pro2_media_button_sample(&s,true,220));
assert(!pro2_media_button_sample(&s,true,919));
assert(pro2_media_button_sample(&s,true,920)==PRO2_ACTION_RESTART);
assert(!pro2_media_button_sample(&s,true,3000));
assert(!pro2_media_button_sample(&s,false,3010));
assert(!pro2_media_button_sample(&s,false,3030));
struct pro2_media_button boot={0};
assert(!pro2_media_button_sample(&boot,true,0));
assert(!pro2_media_button_sample(&boot,true,8000));
assert(!pro2_media_button_sample(&boot,false,8010));
assert(!pro2_media_button_sample(&boot,false,8030));
assert(pro2_knob_delta(255,0x70)==2047);
assert(pro2_knob_delta(0,0x80)==-2048);
assert(pro2_knob_delta(255,0xf0)==-1);
struct pro2_motion_state m={0};
assert(!pro2_motion_volume(&m,-40,false));
assert(pro2_motion_volume(&m,-40,false)==1);
assert(!pro2_motion_volume(&m,40,false));
assert(pro2_motion_volume(&m,40,false)==-1);
assert(!pro2_motion_volume(&m,2,false));
assert(!pro2_motion_volume(&m,0,true));
assert(!pro2_motion_volume(&m,0,true));assert(m.accumulated==0);
assert(pro2_volume_clamp(-1)==0 && pro2_volume_clamp(20)==15);
puts("media controls: short press, hold/release, boot-held, signed motion, direction, idle reset and volume bounds passed");
return 0;
}
