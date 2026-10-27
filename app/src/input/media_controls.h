#ifndef PRO2_MEDIA_CONTROLS_H
#define PRO2_MEDIA_CONTROLS_H
#include "crown_debounce.h"
enum { PRO2_ACTION_NONE=0, PRO2_ACTION_PLAY_PAUSE=1, PRO2_ACTION_RESTART=2 };
struct pro2_media_button {
    struct pro2_crown_state debounce;
    bool active, hold_fired;
    uint32_t pressed_at;
};
static inline int pro2_media_button_sample(struct pro2_media_button *s,bool down,uint32_t now)
{
    if (pro2_crown_sample(&s->debounce,down,now)) {
        s->active=true; s->hold_fired=false; s->pressed_at=now;
    }
    if (s->active && s->debounce.stable && !s->hold_fired &&
        (uint32_t)(now-s->pressed_at)>=700) {
        s->hold_fired=true;
        return PRO2_ACTION_RESTART;
    }
    if (s->active && !s->debounce.stable) {
        s->active=false;
        return s->hold_fired ? PRO2_ACTION_NONE : PRO2_ACTION_PLAY_PAUSE;
    }
    return PRO2_ACTION_NONE;
}
static inline int pro2_knob_delta(uint8_t low,uint8_t high)
{
    int v=((high>>4)<<8)|low;
    return (v&0x800) ? v-0x1000 : v;
}
struct pro2_motion_state { int accumulated; unsigned empty; };
/* Stock negative displacement is clockwise (volume up). */
static inline int pro2_motion_volume(struct pro2_motion_state *s,int delta,bool empty)
{
    if (empty) {
        if (++s->empty>=2) {s->empty=0;s->accumulated=0;return 0;}
    } else s->empty=0;
    s->accumulated+=delta;
    if(s->accumulated>4096)s->accumulated=4096;
    if(s->accumulated< -4096)s->accumulated= -4096;
    if (delta>=3 && s->accumulated>=80) {s->accumulated=0;return -1;}
    if (delta<=-3 && s->accumulated<=-80) {s->accumulated=0;return 1;}
    return 0;
}
static inline int pro2_volume_clamp(int level)
{
    return level<0 ? 0 : level>15 ? 15 : level;
}
#endif
