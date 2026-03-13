/**
 * @file box.c
 * @brief Box widget — a container that draws its background and rounded corners.
 */

#include "box.h"
#include "widget_internal.h"

/* ── Draw ─────────────────────────────────────────────────────────── */

void nk_box_draw(NkWidget *self, NkRenderContext *rc) {
    if (!self || !self->node || !rc || !rc->sdf) return;

    NkStyle *s   = &self->node->style;
    NkRect   rect = nk_widget_abs_rect(self, rc);

    if (rect.w <= 0.0f || rect.h <= 0.0f) return;

    nk_sdf_rect(rc->sdf, rect, s->corner_radius,
                s->background, s->border_width, 1.0f);
}

/* ── Factory ──────────────────────────────────────────────────────── */

NkWidget *nk_box_create(void) {
    static const NkWidgetVTable vt = {
        .type_name    = "Box",
        .draw         = nk_box_draw,
        .layout       = NULL,
        .handle_event = NULL,
        .destroy      = NULL,
    };

    NkBasicWidget *w = (NkBasicWidget *)nk_widget_create(&vt, sizeof(NkBasicWidget));
    if (!w) return NULL;

    w->kind     = NK_WIDGET_BOX;
    w->toggled  = false;
    w->hovered  = false;
    w->pressed  = false;
    w->font_size = 16.0f;
    w->lua_on_click_ref = -2;
    w->text[0]  = '\0';
    w->color_on  = nk_color_hex(0x6750A4FF);
    w->color_off = nk_color_hex(0x79747EFF);

    w->base.node->style.background    = nk_color_hex(0x1F1D23FF);
    w->base.node->style.corner_radius = (NkCorners){ 12, 12, 12, 12 };

    return (NkWidget *)w;
}
