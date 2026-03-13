/**
 * @file node.h
 * @brief Layout tree node — the structure that defines UI hierarchy and style.
 */

#ifndef NK_NODE_H
#define NK_NODE_H

#include <nykhara/types.h>

typedef enum NkFlexDirection {
    NK_FLEX_ROW,
    NK_FLEX_COLUMN,
} NkFlexDirection;

typedef enum NkJustify {
    NK_JUSTIFY_START,
    NK_JUSTIFY_CENTER,
    NK_JUSTIFY_END,
    NK_JUSTIFY_SPACE_BETWEEN,
    NK_JUSTIFY_SPACE_AROUND,
    NK_JUSTIFY_SPACE_EVENLY,
} NkJustify;

typedef enum NkAlign {
    NK_ALIGN_START,
    NK_ALIGN_CENTER,
    NK_ALIGN_END,
    NK_ALIGN_STRETCH,
} NkAlign;

typedef enum NkOverflow {
    NK_OVERFLOW_VISIBLE,
    NK_OVERFLOW_HIDDEN,
    NK_OVERFLOW_SCROLL,
} NkOverflow;

typedef struct NkStyle {
    NkFlexDirection  direction;
    NkJustify        justify;
    NkAlign          align_items;
    NkAlign          align_self;
    NkOverflow       overflow;

    float            width;         // 0 = auto
    float            height;        // 0 = auto
    float            min_width;
    float            min_height;
    float            max_width;     // 0 = unbounded
    float            max_height;

    float            flex_grow;
    float            flex_shrink;
    float            flex_basis;    // 0 = auto

    float            gap;           // Gap between children

    NkEdgeInsets     margin;
    NkEdgeInsets     padding;

    NkCorners        corner_radius;
    NkColor          background;
    NkColor          border_color;
    float            border_width;
} NkStyle;

struct NkNode {
    NkStyle         style;

    // Computed layout (filled after layout pass)
    NkRect          computed;       // Final rect in parent-local space
    NkRect          clip;           // Scissor rect (for overflow)

    // Tree
    NkNode         *parent;
    NkNode         *first_child;
    NkNode         *last_child;
    NkNode         *next_sibling;
    uint32_t        child_count;

    // Back-reference to widget
    NkWidget       *widget;
};

/**
 * Create a layout node.
 */
NkNode *nk_node_create(void);

/**
 * Add a child node.
 */
void nk_node_add_child(NkNode *parent, NkNode *child);

/**
 * Remove a child node from its parent.
 */
void nk_node_remove(NkNode *node);

/**
 * Destroy node and all its children recursively.
 */
void nk_node_destroy(NkNode *node);

#endif // NK_NODE_H

