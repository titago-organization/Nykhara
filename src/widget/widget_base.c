/**
 * @file widget_base.c
 * @brief Widget tree management — creation, destruction, drawing traversal,
 *        animation dispatch, hit testing, accessors.
 *
 * Widget-specific logic (draw, animation, activation) is in:
 *   box.c, button.c, switch_widget.c, label.c
 */

#include "widget_base.h"
#include "box.h"
#include "button.h"
#include "switch_widget.h"
#include "label.h"
#include "../core/nk_log.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

static uint32_t s_next_id = 1;

/* ═══════════════════════════════════════════════════════════════════
 *  Tree management
 * ═══════════════════════════════════════════════════════════════════ */

NkWidget *nk_widget_create(const NkWidgetVTable *vt, size_t size) {
    NkWidget *w = calloc(1, size);
    if (!w) return NULL;

    w->vtable = vt;
    w->id     = s_next_id++;
    w->node   = nk_node_create();
    if (!w->node) {
        free(w);
        return NULL;
    }
    w->node->widget = w;
    return w;
}

void nk_widget_destroy(NkWidget *w) {
    if (!w) return;

    NkWidget *child = w->first_child;
    while (child) {
        NkWidget *next = child->next_sibling;
        nk_widget_destroy(child);
        child = next;
    }

    if (w->vtable && w->vtable->destroy) {
        w->vtable->destroy(w);
    }
    if (w->node) {
        nk_node_destroy(w->node);
    }
    free(w);
}

void nk_widget_add_child(NkWidget *parent, NkWidget *child) {
    if (!parent || !child) return;

    child->parent       = parent;
    child->next_sibling = NULL;

    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        NkWidget *last = parent->first_child;
        while (last->next_sibling) last = last->next_sibling;
        last->next_sibling = child;
    }

    nk_node_add_child(parent->node, child->node);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Drawing
 * ═══════════════════════════════════════════════════════════════════ */

void nk_widget_draw_tree(NkWidget *root, NkRenderContext *rc) {
    if (!root || !rc) return;

    NkRenderContext local = *rc;
    if (root->vtable && root->vtable->draw) {
        root->vtable->draw(root, &local);
    }

    local.offset_x += root->node ? root->node->computed.x : 0.0f;
    local.offset_y += root->node ? root->node->computed.y : 0.0f;

    for (NkWidget *c = root->first_child; c; c = c->next_sibling) {
        nk_widget_draw_tree(c, &local);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  Animation dispatch
 * ═══════════════════════════════════════════════════════════════════ */

void nk_widget_tick_animations(NkWidget *root, float dt) {
    if (!root) return;

    if (dt < 0.0f)  dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;

    NkBasicWidget *bw = (NkBasicWidget *)root;

    switch (bw->kind) {
        case NK_WIDGET_SWITCH: nk_switch_tick(bw, dt); break;
        case NK_WIDGET_BUTTON: nk_button_tick(bw, dt); break;
        default: break;
    }

    for (NkWidget *c = root->first_child; c; c = c->next_sibling) {
        nk_widget_tick_animations(c, dt);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  Hit testing
 * ═══════════════════════════════════════════════════════════════════ */

static NkWidget *hit_test_node(NkWidget *w, float x, float y,
                                float ox, float oy) {
    if (!w || !w->node) return NULL;

    float abs_x = ox + w->node->computed.x;
    float abs_y = oy + w->node->computed.y;

    NkWidget *top = NULL;
    for (NkWidget *c = w->first_child; c; c = c->next_sibling) {
        NkWidget *h = hit_test_node(c, x, y, abs_x, abs_y);
        if (h) top = h;
    }
    if (top) return top;

    if (x >= abs_x && y >= abs_y &&
        x <= abs_x + w->node->computed.w &&
        y <= abs_y + w->node->computed.h) {
        return w;
    }
    return NULL;
}

NkWidget *nk_widget_hit_test(NkWidget *root, float x, float y) {
    return hit_test_node(root, x, y, 0.0f, 0.0f);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Activation
 * ═══════════════════════════════════════════════════════════════════ */

bool nk_widget_activate(NkWidget *w) {
    if (!w) return false;

    NkBasicWidget *bw = (NkBasicWidget *)w;

    switch (bw->kind) {
        case NK_WIDGET_SWITCH: return nk_switch_activate(bw);
        case NK_WIDGET_BUTTON: return nk_button_activate(bw);
        default: return false;
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  Built-in factory
 * ═══════════════════════════════════════════════════════════════════ */

NkWidget *nk_widget_create_builtin(NkWidgetKind kind) {
    switch (kind) {
        case NK_WIDGET_BOX:    return nk_box_create();
        case NK_WIDGET_BUTTON: return nk_button_create();
        case NK_WIDGET_SWITCH: return nk_switch_create();
        case NK_WIDGET_LABEL:  return nk_label_create();
    }
    return NULL;
}

bool nk_widget_kind_from_type(const char *type_name, NkWidgetKind *out_kind) {
    if (!type_name || !out_kind) return false;

    if (strcmp(type_name, "Box") == 0 ||
        strcmp(type_name, "Row") == 0 ||
        strcmp(type_name, "Column") == 0) {
        *out_kind = NK_WIDGET_BOX;
        return true;
    }
    if (strcmp(type_name, "Button") == 0) { *out_kind = NK_WIDGET_BUTTON; return true; }
    if (strcmp(type_name, "Switch") == 0) { *out_kind = NK_WIDGET_SWITCH; return true; }
    if (strcmp(type_name, "Label") == 0)  { *out_kind = NK_WIDGET_LABEL;  return true; }
    return false;
}

void nk_widget_apply_style_defaults(NkWidget *w) {
    if (!w || !w->node) return;

    NkStyle *s = &w->node->style;
    if (s->background.a <= 0.0f) {
        s->background = nk_color_hex(0x00000000);
    }

    NkBasicWidget *bw = (NkBasicWidget *)w;
    if (bw->kind == NK_WIDGET_SWITCH) {
        bw->color_off = s->background.a > 0.0f
                            ? s->background
                            : nk_color_hex(0x79747EFF);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  Accessors
 * ═══════════════════════════════════════════════════════════════════ */

void nk_widget_set_text(NkWidget *w, const char *text) {
    if (!w || !text) return;
    NkBasicWidget *bw = (NkBasicWidget *)w;
    strncpy(bw->text, text, sizeof(bw->text) - 1);
    bw->text[sizeof(bw->text) - 1] = '\0';
}

const char *nk_widget_text(const NkWidget *w) {
    if (!w) return "";
    return ((const NkBasicWidget *)w)->text;
}

void nk_widget_set_lua_callback_ref(NkWidget *w, int ref) {
    if (!w) return;
    ((NkBasicWidget *)w)->lua_on_click_ref = ref;
}

int nk_widget_lua_callback_ref(const NkWidget *w) {
    if (!w) return -2;
    return ((const NkBasicWidget *)w)->lua_on_click_ref;
}
