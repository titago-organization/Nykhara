/**
 * @file label.h
 * @brief Label widget — text display with optional background.
 */

#ifndef NK_WIDGET_LABEL_H
#define NK_WIDGET_LABEL_H

#include "widget_base.h"

void     nk_label_draw(NkWidget *self, NkRenderContext *rc);
NkWidget *nk_label_create(void);

#endif /* NK_WIDGET_LABEL_H */
