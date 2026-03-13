/**
 * @file button.c
 * @brief Button widget — pulsing background, press animation, centered text.
 */

#include "button.h"
#include "widget_internal.h"

#include <string.h>
#include <math.h>

/* ── Draw ─────────────────────────────────────────────────────────── */

void nk_button_draw(NkWidget *self, NkRenderContext *rc) {
    if (!self || !self->node || !rc || !rc->sdf) return;

    NkBasicWidget *bw = (NkBasicWidget *)self;
    NkStyle       *s  = &self->node->style;
    NkRect         rect = nk_widget_abs_rect(self, rc);

    if (rect.w <= 0.0f || rect.h <= 0.0f) return;

    /* Subtle breathing pulse + press/hover modulation */
    float pulse = 0.94f + 0.06f * (float)(0.5 + 0.5 * sin(rc->time_sec * 2.0));
    if (bw->pressed) pulse *= 0.88f;
    if (bw->hovered) pulse *= 1.04f;

    NkColor fill = nk_color_mul_rgb(s->background, pulse);
    nk_sdf_rect(rc->sdf, rect, s->corner_radius, fill, s->border_width, 1.0f);

    /* Centered text */
    float px     = bw->font_size > 0.0f ? (bw->font_size / 7.0f) : 2.2f;
    float text_w = (float)strlen(bw->text) * px * 6.0f;
    float text_h = px * 7.0f;
    float tx     = rect.x + (rect.w - text_w) * 0.5f;
    float ty     = rect.y + (rect.h - text_h) * 0.5f;

    nk_text_draw(rc->sdf, tx, ty, px, bw->text, nk_color_hex(0xF8F5FFFF));
}

/* ── Animation ────────────────────────────────────────────────────── */

void nk_button_tick(NkBasicWidget *bw, float dt) {
    float target = bw->pressed ? 1.0f : 0.0f;
    bw->anim_c += (target - bw->anim_c) * nk_exp_smooth(28.0f, dt);
}

/* ── Activation ───────────────────────────────────────────────────── */

bool nk_button_activate(NkBasicWidget *bw) {
    bw->anim_b = 1.0f;
    return true;
}

/* ── Factory ──────────────────────────────────────────────────────── */

NkWidget *nk_button_create(void) {
    static const NkWidgetVTable vt = {
        .type_name    = "Button",
        .draw         = nk_button_draw,
        .layout       = NULL,
        .handle_event = NULL,
        .destroy      = NULL,
    };

    NkBasicWidget *w = (NkBasicWidget *)nk_widget_create(&vt, sizeof(NkBasicWidget));
    if (!w) return NULL;

    w->kind     = NK_WIDGET_BUTTON;
    w->toggled  = false;
    w->hovered  = false;
    w->pressed  = false;
    w->font_size = 16.0f;
    w->lua_on_click_ref = -2;
    w->color_on  = nk_color_hex(0x6750A4FF);
    w->color_off = nk_color_hex(0x79747EFF);

    strncpy(w->text, "Button", sizeof(w->text) - 1);

    w->base.node->style.background    = nk_color_hex(0x6750A4FF);
    w->base.node->style.corner_radius = (NkCorners){ 16, 16, 16, 16 };
    w->base.node->style.height        = 46.0f;
    w->base.node->style.width         = 160.0f;

    return (NkWidget *)w;
}
