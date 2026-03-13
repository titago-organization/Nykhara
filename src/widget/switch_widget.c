//
// Created by amapekibert on 3/13/26.
//

#include "switch_widget.h"
#include "widget_internal.h"

#include <math.h>

// drawing:

void nk_switch_draw(NkWidget *self, NkRenderContext *rc) {
    if (!self || !self->node || !rc || !rc->sdf) return;

    NkBasicWidget *bw = (NkBasicWidget *)self;
    NkRect rect = nk_widget_abs_rect(self, rc);

    float h = rect.h > 0.0f ? rect.h : 32.0f;
    float w = rect.w > 0.0f ? rect.w : (h * 1.625f); 
    rect.w = w;
    rect.h = h;

    float t_raw = bw->anim_a; 
    float t_sat = nk_saturatef(t_raw);
    float t = nk_smoothstep01(t_sat); 
    float r = h * 0.5f; 

    // track:
    NkColor off_outline_color = nk_color_hex(0x938F99FF); 
    NkColor off_track_fill    = nk_color_hex(0x49454FFF); 
    NkColor on_track_fill     = bw->color_on;

    float max_border = (h * 0.1f < 2.0f) ? 2.0f : h * 0.1f;
    float border_thickness = max_border * (1.0f - t);

    NkColor track_color = {
        .r = off_track_fill.r + (on_track_fill.r - off_track_fill.r) * t,
        .g = off_track_fill.g + (on_track_fill.g - off_track_fill.g) * t,
        .b = off_track_fill.b + (on_track_fill.b - off_track_fill.b) * t,
        .a = 1.0f
    };

    nk_sdf_rect(rc->sdf, rect, (NkCorners){ r, r, r, r }, track_color, 0.0f, 1.0f);

    if (border_thickness > 0.1f) {
        nk_sdf_rect(rc->sdf, rect, (NkCorners){ r, r, r, r }, off_outline_color, border_thickness, 1.0f);
    }

    // circle
    float velocity = bw->anim_b;
    float press_t  = bw->anim_c;
    
    float thumb_r_off = h * 0.25f; 
    float thumb_r_on  = h * 0.375f;
    float base_thumb_r = thumb_r_off + (thumb_r_on - thumb_r_off) * t;

    // circle correction
    float start_x = rect.x + r;           
    float end_x   = rect.x + rect.w - r - (h * 0.01f); 
    float thumb_cx = start_x + (end_x - start_x) * t_raw; 
    float thumb_cy = rect.y + r;

    NkColor thumb_color_off = nk_color_hex(0x938F99FF); 
    NkColor thumb_color_on  = nk_color_hex(0x381E72FF);
    NkColor thumb_color = {
        .r = thumb_color_off.r + (thumb_color_on.r - thumb_color_off.r) * t,
        .g = thumb_color_off.g + (thumb_color_on.g - thumb_color_off.g) * t,
        .b = thumb_color_off.b + (thumb_color_on.b - thumb_color_off.b) * t,
        .a = 1.0f
    };

    float impulse_abs = nk_saturatef(fabsf(velocity) * 0.05f);
    float squash  = 1.0f - 0.12f * press_t - 0.08f * impulse_abs;
    float stretch = 1.0f + 0.12f * press_t + 0.15f * impulse_abs;
    
    float thumb_w = 2.0f * base_thumb_r * stretch;
    float thumb_h = 2.0f * base_thumb_r * squash;
    float thumb_cr = thumb_h * 0.5f;

    nk_sdf_rect(rc->sdf,
                (NkRect){ thumb_cx - thumb_w * 0.5f,
                          thumb_cy - thumb_h * 0.5f,
                          thumb_w, thumb_h },
                (NkCorners){ thumb_cr, thumb_cr, thumb_cr, thumb_cr },
                thumb_color, 0.0f, 1.0f);
}

// Animation logic

void nk_switch_tick(NkBasicWidget *bw, float dt) {
    float target = bw->toggled ? 1.0f : 0.0f;
    const float omega = 22.0f;
    const float zeta  = 0.65f;
    float x = bw->anim_a;
    float v = bw->anim_b;

    float acc = omega * omega * (target - x) - 2.0f * zeta * omega * v;
    v += acc * dt;
    x += v * dt;

    if (fabsf(target - x) < 0.0001f && fabsf(v) < 0.001f) {
        x = target;
        v = 0.0f;
    }

    bw->anim_a = x;
    bw->anim_b = v;

    float press_target = bw->pressed ? 1.0f : 0.0f;
    bw->anim_c += (press_target - bw->anim_c) * nk_exp_smooth(30.0f, dt);
}

bool nk_switch_activate(NkBasicWidget *bw) {
    bw->toggled = !bw->toggled;
    bw->anim_b += bw->toggled ? 2.5f : -2.5f; // springing
    return true;
}

NkWidget *nk_switch_create(void) {
    static const NkWidgetVTable vt = {
        .type_name    = "Switch",
        .draw         = nk_switch_draw,
        .layout       = NULL,
        .handle_event = NULL,
        .destroy      = NULL,
    };

    NkBasicWidget *w = (NkBasicWidget *)nk_widget_create(&vt, sizeof(NkBasicWidget));
    if (!w) return NULL;

    w->kind      = NK_WIDGET_SWITCH;
    w->toggled   = false;
    w->color_on  = nk_color_hex(0xD0BCFEFF);
    w->color_off = nk_color_hex(0x49454FFF);

    w->base.node->style.height = 32.0f;
    w->base.node->style.width  = 52.0f;

    return (NkWidget *)w;
}