#ifndef PRO2_CROWN_DEBOUNCE_H
#define PRO2_CROWN_DEBOUNCE_H
#include <stdint.h>
#include <stdbool.h>
struct pro2_crown_state {
    bool initialized, candidate, stable, armed;
    uint32_t changed;
};
/* Emit once per debounced press. A crown held during boot must be released. */
static inline bool pro2_crown_sample(struct pro2_crown_state *s, bool down, uint32_t now)
{
    if (!s->initialized) {
        s->initialized=true; s->candidate=down; s->stable=down;
        s->armed=!down; s->changed=now;
        return false;
    }
    if (down != s->candidate) { s->candidate=down; s->changed=now; }
    if (s->stable != s->candidate && (uint32_t)(now-s->changed) >= 20) {
        s->stable=s->candidate;
        if (!s->stable) s->armed=true;
        else if (s->armed) { s->armed=false; return true; }
    }
    return false;
}
#endif
