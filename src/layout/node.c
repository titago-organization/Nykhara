/**
 * @file node.c
 * @brief Layout node tree management.
 */

#include "node.h"
#include "../core/nk_log.h"

#include <stdlib.h>
#include <string.h>

NkNode *nk_node_create(void) {
    NkNode *n = calloc(1, sizeof(NkNode));
    if (!n) return NULL;
    // Defaults
    n->style.direction   = NK_FLEX_COLUMN;
    n->style.justify     = NK_JUSTIFY_START;
    n->style.align_items = NK_ALIGN_STRETCH;
    n->style.flex_shrink = 1.0f;
    n->style.overflow    = NK_OVERFLOW_VISIBLE;
    return n;
}

void nk_node_add_child(NkNode *parent, NkNode *child) {
    if (!parent || !child) return;
    child->parent = parent;
    child->next_sibling = NULL;

    if (!parent->first_child) {
        parent->first_child = child;
        parent->last_child  = child;
    } else {
        parent->last_child->next_sibling = child;
        parent->last_child = child;
    }
    parent->child_count++;
}

void nk_node_remove(NkNode *node) {
    if (!node || !node->parent) return;
    NkNode *parent = node->parent;

    NkNode *prev = NULL;
    for (NkNode *c = parent->first_child; c; c = c->next_sibling) {
        if (c == node) {
            if (prev) {
                prev->next_sibling = c->next_sibling;
            } else {
                parent->first_child = c->next_sibling;
            }
            if (c == parent->last_child) {
                parent->last_child = prev;
            }
            parent->child_count--;
            break;
        }
        prev = c;
    }
    node->parent = NULL;
    node->next_sibling = NULL;
}

void nk_node_destroy(NkNode *node) {
    if (!node) return;
    // Destroy children first
    NkNode *child = node->first_child;
    while (child) {
        NkNode *next = child->next_sibling;
        nk_node_destroy(child);
        child = next;
    }
    free(node);
}

