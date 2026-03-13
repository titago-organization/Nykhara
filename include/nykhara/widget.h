/**
 * @file widget.h
 * @brief Public widget vtable — base interface for all widgets.
 */

#ifndef NK_WIDGET_H
#define NK_WIDGET_H

#include "types.h"

// Forward declarations
typedef struct NkRenderContext NkRenderContext;
typedef struct NkEvent         NkEvent;
typedef struct NkLayoutContext  NkLayoutContext;

/**
 * Every widget type provides this vtable.
 */
typedef struct NkWidgetVTable {
    const char *type_name;

    void     (*draw)         (NkWidget *self, NkRenderContext *rc);
    void     (*layout)       (NkWidget *self, NkLayoutContext *lc);
    bool     (*handle_event) (NkWidget *self, const NkEvent *event);
    void     (*destroy)      (NkWidget *self);
} NkWidgetVTable;

/**
 * Base widget struct — every concrete widget embeds this as first member.
 */
struct NkWidget {
    const NkWidgetVTable *vtable;
    NkNode               *node;       // Layout node
    NkWidget             *parent;
    NkWidget             *first_child;
    NkWidget             *next_sibling;
    uint32_t              id;
    uint32_t              flags;
    void                 *user_data;
};

// Widget tree helpers
NkWidget *nk_widget_create  (const NkWidgetVTable *vt, size_t size);
void      nk_widget_destroy (NkWidget *w);
void      nk_widget_add_child(NkWidget *parent, NkWidget *child);

#endif // NK_WIDGET_H

