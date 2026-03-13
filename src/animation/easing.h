/**
 * @file easing.h
 * @brief Easing functions for animations — cubic-bezier, spring, standard Material curves.
 */

#ifndef NK_EASING_H
#define NK_EASING_H

typedef float (*NkEasingFn)(float t);

float nk_ease_linear(float t);
float nk_ease_in_quad(float t);
float nk_ease_out_quad(float t);
float nk_ease_in_out_quad(float t);
float nk_ease_in_cubic(float t);
float nk_ease_out_cubic(float t);
float nk_ease_in_out_cubic(float t);
float nk_ease_out_expo(float t);
float nk_ease_out_elastic(float t);
float nk_ease_out_bounce(float t);

/**
 * Cubic bezier easing (like CSS cubic-bezier).
 * @param p1x, p1y, p2x, p2y  Control points.
 * @param t  Progress [0..1].
 */
float nk_ease_cubic_bezier(float p1x, float p1y, float p2x, float p2y, float t);

/**
 * Material Design 3 standard easing curve.
 */
float nk_ease_md3_standard(float t);

/**
 * Material Design 3 emphasized easing curve.
 */
float nk_ease_md3_emphasized(float t);

#endif // NK_EASING_H

