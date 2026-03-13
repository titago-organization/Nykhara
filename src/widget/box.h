/**
 * @file box.h
 * @brief Box widget — generic container with background and rounded corners.
 */

#ifndef NK_WIDGET_BOX_H
#define NK_WIDGET_BOX_H

#include "widget_base.h"

/**
 * Draw callback for box widgets.
 */
void nk_box_draw(NkWidget *self, NkRenderContext *rc);

/**
 * Allocate and configure a default box widget.
 */
NkWidget *nk_box_create(void);

#endif /* NK_WIDGET_BOX_H */
