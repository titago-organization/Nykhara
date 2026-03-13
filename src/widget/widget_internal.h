/**
 * @file widget_internal.h
 * @brief Internal helpers shared across widget implementations.
 *
 * Not part of the public API — only included by widget source files.
 */

#ifndef NK_WIDGET_INTERNAL_H
#define NK_WIDGET_INTERNAL_H

#include "widget_base.h"
#include "text_render.h"

#include <math.h>

/* ── Math helpers ─────────────────────────────────────────────────── */

static inline float nk_saturatef(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline float nk_smoothstep01(float t) {
    t = nk_saturatef(t);
    return t * t * (3.0f - 2.0f * t);
}

static inline float nk_exp_smooth(float rate, float dt) {
    return 1.0f - expf(-rate * dt);
}

/* ── Colour helpers ───────────────────────────────────────────────── */

static inline NkColor nk_color_mul_rgb(NkColor c, float k) {
    c.r *= k;
    c.g *= k;
    c.b *= k;
    return c;
}

/* ── Geometry helpers ─────────────────────────────────────────────── */

static inline NkRect nk_widget_abs_rect(const NkWidget *w,
                                         const NkRenderContext *rc) {
    return (NkRect){
        rc->offset_x + w->node->computed.x,
        rc->offset_y + w->node->computed.y,
        w->node->computed.w,
        w->node->computed.h,
    };
}

#endif /* NK_WIDGET_INTERNAL_H */
