/**
 * @file flex.h
 * @brief Flexbox-like layout solver.
 */

#ifndef NK_FLEX_H
#define NK_FLEX_H

#include "node.h"

/**
 * Compute layout for a tree starting at `root`.
 * @param root    Root layout node.
 * @param width   Available width.
 * @param height  Available height.
 */
void nk_flex_layout(NkNode *root, float width, float height);

#endif // NK_FLEX_H

