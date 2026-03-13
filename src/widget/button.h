/**
 * @file button.h
 * @brief Button widget — animated filled button with centered text.
 */

#ifndef NK_WIDGET_BUTTON_H
#define NK_WIDGET_BUTTON_H

#include "widget_base.h"

void     nk_button_draw(NkWidget *self, NkRenderContext *rc);
void     nk_button_tick(NkBasicWidget *bw, float dt);
bool     nk_button_activate(NkBasicWidget *bw);
NkWidget *nk_button_create(void);

#endif /* NK_WIDGET_BUTTON_H */
