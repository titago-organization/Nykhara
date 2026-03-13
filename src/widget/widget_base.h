/**
 * @file widget_base.h
 * @brief Base widget helpers — creation, destruction, tree management.
 *
 * Widget-specific draw/animation/activation logic lives in the per-widget
 * headers (box.h, button.h, switch_widget.h, label.h).
 */

#ifndef NK_WIDGET_BASE_H
#define NK_WIDGET_BASE_H

#include <nykhara/widget.h>
#include "../layout/node.h"
#include "../render/sdf_renderer.h"

/* ── Render context ───────────────────────────────────────────────── */

typedef struct NkRenderContext {
    NkSdfRenderer *sdf;
    float          offset_x;
    float          offset_y;
    double         time_sec;
} NkRenderContext;

/* ── Widget kinds ─────────────────────────────────────────────────── */

typedef enum NkWidgetKind {
    NK_WIDGET_BOX,
    NK_WIDGET_BUTTON,
    NK_WIDGET_SWITCH,
    NK_WIDGET_LABEL,
} NkWidgetKind;

/* ── Concrete widget struct ───────────────────────────────────────── */

typedef struct NkBasicWidget {
    NkWidget      base;
    NkWidgetKind  kind;

    NkColor       color_on;
    NkColor       color_off;

    bool          toggled;
    bool          hovered;
    bool          pressed;

    float         font_size;
    int           lua_on_click_ref;

    char          text[128];

    /* Animation slots (meaning varies per widget kind) */
    float         anim_a;
    float         anim_b;
    float         anim_c;
} NkBasicWidget;

/* ── Tree management ──────────────────────────────────────────────── */

NkWidget *nk_widget_create(const NkWidgetVTable *vt, size_t size);
void      nk_widget_destroy(NkWidget *w);
void      nk_widget_add_child(NkWidget *parent, NkWidget *child);

/* ── Drawing & layout ─────────────────────────────────────────────── */

void nk_widget_draw_tree(NkWidget *root, NkRenderContext *rc);
void nk_widget_tick_animations(NkWidget *root, float dt);

/* ── Built-in creation ────────────────────────────────────────────── */

NkWidget *nk_widget_create_builtin(NkWidgetKind kind);
bool      nk_widget_kind_from_type(const char *type_name, NkWidgetKind *out_kind);
void      nk_widget_apply_style_defaults(NkWidget *w);

/* ── Hit testing & activation ─────────────────────────────────────── */

NkWidget *nk_widget_hit_test(NkWidget *root, float x, float y);
bool      nk_widget_activate(NkWidget *w);

/* ── Accessors ────────────────────────────────────────────────────── */

void        nk_widget_set_text(NkWidget *w, const char *text);
const char *nk_widget_text(const NkWidget *w);
void        nk_widget_set_lua_callback_ref(NkWidget *w, int ref);
int         nk_widget_lua_callback_ref(const NkWidget *w);

#endif /* NK_WIDGET_BASE_H */
