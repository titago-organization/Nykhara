/**
 * @file fallback_ui.c
 * @brief Default widget tree shown when Lua scripting is unavailable.
 */

#include "fallback_ui.h"

NkWidget *nk_build_fallback_ui(void) {
    NkWidget *root = nk_widget_create_builtin(NK_WIDGET_BOX);
    if (!root) return NULL;

    root->node->style.direction  = NK_FLEX_COLUMN;
    root->node->style.gap        = 12.0f;
    root->node->style.padding    = (NkEdgeInsets){ 22, 22, 22, 22 };
    root->node->style.background = nk_color_hex(0x121018FF);

    NkWidget *fps = nk_widget_create_builtin(NK_WIDGET_LABEL);
    NkWidget *row = nk_widget_create_builtin(NK_WIDGET_BOX);
    NkWidget *btn = nk_widget_create_builtin(NK_WIDGET_BUTTON);
    NkWidget *sw  = nk_widget_create_builtin(NK_WIDGET_SWITCH);

    if (!fps || !row || !btn || !sw) {
        if (fps) nk_widget_destroy(fps);
        if (row) nk_widget_destroy(row);
        if (btn) nk_widget_destroy(btn);
        if (sw)  nk_widget_destroy(sw);
        nk_widget_destroy(root);
        return NULL;
    }

    nk_widget_set_text(fps, "FPS: 0");
    ((NkBasicWidget *)fps)->font_size = 15.0f;

    row->node->style.direction     = NK_FLEX_ROW;
    row->node->style.gap           = 12.0f;
    row->node->style.height        = 56.0f;
    row->node->style.background    = nk_color_hex(0x1F1D23FF);
    row->node->style.corner_radius = (NkCorners){ 14, 14, 14, 14 };
    row->node->style.padding       = (NkEdgeInsets){ 8, 12, 8, 12 };

    nk_widget_set_text(btn, "Primary");
    ((NkBasicWidget *)sw)->toggled = true;

    nk_widget_add_child(root, fps);
    nk_widget_add_child(root, row);
    nk_widget_add_child(row, btn);
    nk_widget_add_child(row, sw);

    return root;
}
