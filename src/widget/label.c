/**
 * @file label.c
 * @brief Label widget — renders a text string with an optional background.
 */

#include "label.h"
#include "widget_internal.h"

#include <string.h>

/* ── Draw ─────────────────────────────────────────────────────────── */

void nk_label_draw(NkWidget *self, NkRenderContext *rc) {
    if (!self || !self->node || !rc || !rc->sdf) return;

    NkBasicWidget *bw = (NkBasicWidget *)self;
    NkStyle       *s  = &self->node->style;
    NkRect         rect = nk_widget_abs_rect(self, rc);

    if (rect.w <= 0.0f || rect.h <= 0.0f) return;

    /* Optional background */
    if (s->background.a > 0.0f) {
        nk_sdf_rect(rc->sdf, rect, s->corner_radius,
                    s->background, s->border_width, 1.0f);
    }

    /* Text */
    float px = bw->font_size > 0.0f ? (bw->font_size / 7.0f) : 2.2f;
    float tx = rect.x + 2.0f;
    float ty = rect.y + (rect.h > px * 7.0f ? (rect.h - px * 7.0f) * 0.5f : 0.0f);

    NkColor tc = bw->color_on.a > 0.0f ? bw->color_on : nk_color_hex(0xE6E1E5FF);
    nk_text_draw(rc->sdf, tx, ty, px, bw->text, tc);
}

/* ── Factory ──────────────────────────────────────────────────────── */

NkWidget *nk_label_create(void) {
    static const NkWidgetVTable vt = {
        .type_name    = "Label",
        .draw         = nk_label_draw,
        .layout       = NULL,
        .handle_event = NULL,
        .destroy      = NULL,
    };

    NkBasicWidget *w = (NkBasicWidget *)nk_widget_create(&vt, sizeof(NkBasicWidget));
    if (!w) return NULL;

    w->kind     = NK_WIDGET_LABEL;
    w->toggled  = false;
    w->hovered  = false;
    w->pressed  = false;
    w->font_size = 16.0f;
    w->lua_on_click_ref = -2;
    w->color_on  = nk_color_hex(0xE6E1E5FF);
    w->color_off = nk_color_hex(0x79747EFF);

    strncpy(w->text, "Label", sizeof(w->text) - 1);

    w->base.node->style.background = nk_color_hex(0x00000000);
    w->base.node->style.height     = 24.0f;
    w->base.node->style.width      = 180.0f;

    return (NkWidget *)w;
}
