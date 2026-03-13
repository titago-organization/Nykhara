/**
 * @file morph_demo.c
 * @brief Animated morph shape — keyframed transitions between triangle,
 *        square, and circle with colour blending.
 */

#include "morph_demo.h"

#include <math.h>

/* ── Colour unpacking ─────────────────────────────────────────────── */

static void color_unpack(uint32_t hex,
                         float *r, float *g, float *b, float *a) {
    *r = (float)((hex >> 24) & 0xFF) / 255.0f;
    *g = (float)((hex >> 16) & 0xFF) / 255.0f;
    *b = (float)((hex >>  8) & 0xFF) / 255.0f;
    *a = (float)((hex >>  0) & 0xFF) / 255.0f;
}

/* ── Public API ───────────────────────────────────────────────────── */

NkMorphState nk_morph_state(float time_sec) {
    const float seg_dur = 3.2f;
    const float total   = seg_dur * 4.0f;

    float morph_keyframes[5]  = { 0.0f, 1.0f, 2.0f, 1.0f, 0.0f };
    float corner_keyframes[5] = { 0.0f, 4.0f, 0.0f, 4.0f, 0.0f };
    uint32_t color_keyframes[5] = {
        0xD0BCFFFF,
        0x6750A4FF,
        0xEADDFFFF,
        0x6750A4FF,
        0xD0BCFFFF,
    };

    float cycle = fmodf(time_sec, total);
    int   seg   = (int)(cycle / seg_dur);
    if (seg > 3) seg = 3;

    float local = (cycle - (float)seg * seg_dur) / seg_dur;
    float t     = (1.0f - cosf(local * 3.14159265f)) * 0.5f;

    NkMorphState ms;
    ms.morph_t  = nk_lerpf(morph_keyframes[seg],  morph_keyframes[seg + 1],  t);
    ms.corner_r = nk_lerpf(corner_keyframes[seg], corner_keyframes[seg + 1], t);

    float ca[4], cb[4];
    color_unpack(color_keyframes[seg],     &ca[0], &ca[1], &ca[2], &ca[3]);
    color_unpack(color_keyframes[seg + 1], &cb[0], &cb[1], &cb[2], &cb[3]);
    ms.color_r = nk_lerpf(ca[0], cb[0], t);
    ms.color_g = nk_lerpf(ca[1], cb[1], t);
    ms.color_b = nk_lerpf(ca[2], cb[2], t);
    ms.color_a = nk_lerpf(ca[3], cb[3], t);

    return ms;
}
