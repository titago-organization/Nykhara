/**
 * @file morph.h
 * @brief Shape morphing — smooth vertex interpolation between shapes.
 *
 * Enables square→circle→triangle and arbitrary polygon transitions.
 * Shapes are represented as N control points; morphing resamples
 * both source and target to the same vertex count, then lerps.
 */

#ifndef NK_MORPH_H
#define NK_MORPH_H

#include <nykhara/types.h>

#define NK_MORPH_MAX_VERTICES 128

typedef struct NkMorphShape {
    NkVec2   vertices[NK_MORPH_MAX_VERTICES];
    uint32_t count;
} NkMorphShape;

/**
 * Generate a regular polygon shape with N sides inscribed in a circle.
 * @param center  Center of the shape.
 * @param radius  Radius.
 * @param sides   Number of sides (3=triangle, 4=square, 64≈circle).
 * @param out     Output shape.
 */
void nk_morph_polygon(NkVec2 center, float radius, uint32_t sides, NkMorphShape *out);

/**
 * Generate a circle shape (high vertex count polygon).
 */
void nk_morph_circle(NkVec2 center, float radius, NkMorphShape *out);

/**
 * Generate a rounded rect shape.
 */
void nk_morph_rounded_rect(NkRect rect, float corner_radius, NkMorphShape *out);

/**
 * Resample a shape to a target vertex count (for morphing between different vertex counts).
 */
void nk_morph_resample(const NkMorphShape *src, uint32_t target_count, NkMorphShape *out);

/**
 * Lerp between two shapes (must have same vertex count — call resample first).
 * @param a, b  Source and target shapes.
 * @param t     Interpolation factor [0..1].
 * @param out   Output interpolated shape.
 */
void nk_morph_lerp(const NkMorphShape *a, const NkMorphShape *b, float t, NkMorphShape *out);

#endif // NK_MORPH_H

