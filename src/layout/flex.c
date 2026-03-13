/**
 * @file flex.c
 * @brief Flexbox-like single-pass layout solver.
 *
 * Supports: direction (row/column), justify-content, align-items,
 *           padding, margin, gap, flex-grow/shrink, min/max sizes.
 */

#include "flex.h"
#include "../core/nk_log.h"

#include <math.h>

static float clamp_size(float val, float min_v, float max_v) {
    if (val < min_v) val = min_v;
    if (max_v > 0 && val > max_v) val = max_v;
    return val;
}

static void layout_node(NkNode *node, float avail_w, float avail_h);

static void layout_children_flex(NkNode *node, float inner_w, float inner_h) {
    NkStyle *s = &node->style;
    bool is_row = (s->direction == NK_FLEX_ROW);

    float main_avail = is_row ? inner_w : inner_h;
    float cross_avail = is_row ? inner_h : inner_w;

    // First pass: measure children, compute total used + total grow/shrink
    float total_fixed = 0;
    float total_grow  = 0;
    uint32_t child_count = node->child_count;

    for (NkNode *c = node->first_child; c; c = c->next_sibling) {
        NkStyle *cs = &c->style;
        float basis = cs->flex_basis > 0 ? cs->flex_basis :
                      (is_row ? cs->width : cs->height);
        if (basis <= 0) basis = 0;
        total_fixed += basis;
        total_grow  += cs->flex_grow;

        // Account for margins
        if (is_row) {
            total_fixed += cs->margin.left + cs->margin.right;
        } else {
            total_fixed += cs->margin.top + cs->margin.bottom;
        }
    }

    // Gap between children
    float total_gap = (child_count > 1) ? s->gap * (float)(child_count - 1) : 0;
    float remaining = main_avail - total_fixed - total_gap;
    if (remaining < 0) remaining = 0;

    // Second pass: distribute space and compute positions
    float cursor = 0;

    // Justify-content: adjust starting cursor
    if (total_grow == 0) {
        switch (s->justify) {
            case NK_JUSTIFY_CENTER:        cursor = remaining / 2.0f; break;
            case NK_JUSTIFY_END:           cursor = remaining; break;
            case NK_JUSTIFY_SPACE_BETWEEN:
                if (child_count > 1)
                    total_gap = remaining / (float)(child_count - 1);
                break;
            case NK_JUSTIFY_SPACE_AROUND:
                if (child_count > 0) {
                    float space = remaining / (float)child_count;
                    cursor = space / 2.0f;
                    total_gap = space;
                }
                break;
            case NK_JUSTIFY_SPACE_EVENLY:
                if (child_count > 0) {
                    float space = remaining / (float)(child_count + 1);
                    cursor = space;
                    total_gap = space;
                }
                break;
            default: break;
        }
    }

    for (NkNode *c = node->first_child; c; c = c->next_sibling) {
        NkStyle *cs = &c->style;

        float basis = cs->flex_basis > 0 ? cs->flex_basis :
                      (is_row ? cs->width : cs->height);
        if (basis <= 0) basis = 0;

        // Flex grow
        float main_size = basis;
        if (total_grow > 0 && cs->flex_grow > 0) {
            main_size += (remaining * cs->flex_grow / total_grow);
        }
        main_size = clamp_size(main_size, is_row ? cs->min_width : cs->min_height,
                                          is_row ? cs->max_width : cs->max_height);

        // Cross size
        float cross_size = is_row ? cs->height : cs->width;
        if (cross_size <= 0 && s->align_items == NK_ALIGN_STRETCH) {
            cross_size = cross_avail;
        }
        if (cross_size <= 0) cross_size = 0;

        // Margins
        float m_before = is_row ? cs->margin.left : cs->margin.top;
        float m_after  = is_row ? cs->margin.right : cs->margin.bottom;
        float m_cross_before = is_row ? cs->margin.top : cs->margin.left;

        float main_pos  = cursor + m_before;
        float cross_pos = m_cross_before;

        // Align-items on cross axis
        if (s->align_items == NK_ALIGN_CENTER) {
            cross_pos = (cross_avail - cross_size) / 2.0f;
        } else if (s->align_items == NK_ALIGN_END) {
            cross_pos = cross_avail - cross_size - m_cross_before;
        }

        // Set computed rect
        if (is_row) {
            c->computed = (NkRect){ main_pos, cross_pos, main_size, cross_size };
        } else {
            c->computed = (NkRect){ cross_pos, main_pos, cross_size, main_size };
        }

        // Recurse
        layout_node(c, c->computed.w, c->computed.h);

        cursor = main_pos + main_size + m_after;
        if (s->justify == NK_JUSTIFY_SPACE_BETWEEN ||
            s->justify == NK_JUSTIFY_SPACE_AROUND ||
            s->justify == NK_JUSTIFY_SPACE_EVENLY) {
            cursor += total_gap;
        } else {
            cursor += s->gap;
        }
    }
}

static void layout_node(NkNode *node, float avail_w, float avail_h) {
    NkStyle *s = &node->style;

    // Inner size (after padding)
    float inner_w = avail_w - s->padding.left - s->padding.right;
    float inner_h = avail_h - s->padding.top  - s->padding.bottom;
    if (inner_w < 0) inner_w = 0;
    if (inner_h < 0) inner_h = 0;

    if (node->first_child) {
        layout_children_flex(node, inner_w, inner_h);

        // Offset children by padding
        for (NkNode *c = node->first_child; c; c = c->next_sibling) {
            c->computed.x += s->padding.left;
            c->computed.y += s->padding.top;
        }
    }
}

void nk_flex_layout(NkNode *root, float width, float height) {
    if (!root) return;
    root->computed = (NkRect){ 0, 0, width, height };
    layout_node(root, width, height);
}

