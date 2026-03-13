/**
 * @file switch_widget.h
 * @brief Toggle-switch widget — animated track + thumb with spring physics.
 */

#ifndef NK_WIDGET_SWITCH_H
#define NK_WIDGET_SWITCH_H

#include "widget_base.h"

void     nk_switch_draw(NkWidget *self, NkRenderContext *rc);
void     nk_switch_tick(NkBasicWidget *bw, float dt);
bool     nk_switch_activate(NkBasicWidget *bw);
NkWidget *nk_switch_create(void);

#endif /* NK_WIDGET_SWITCH_H */
