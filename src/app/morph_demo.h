/**
 * @file morph_demo.h
 * @brief Morph animation state — cycles through shape/colour keyframes.
 */

#ifndef NK_MORPH_DEMO_H
#define NK_MORPH_DEMO_H

#include <nykhara/types.h>

typedef struct NkMorphState {
    float morph_t;
    float corner_r;
    float color_r, color_g, color_b, color_a;
} NkMorphState;

/**
 * Compute the morph state for a given time (seconds since start).
 */
NkMorphState nk_morph_state(float time_sec);

/**
 * Helper: linearly interpolate two floats.
 */
static inline float nk_lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

#endif /* NK_MORPH_DEMO_H */
