/**
 * @file easing.c
 * @brief Easing function implementations.
 */

#include "easing.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float nk_ease_linear(float t) {
    return t;
}

float nk_ease_in_quad(float t) {
    return t * t;
}

float nk_ease_out_quad(float t) {
    return t * (2.0f - t);
}

float nk_ease_in_out_quad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float nk_ease_in_cubic(float t) {
    return t * t * t;
}

float nk_ease_out_cubic(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

float nk_ease_in_out_cubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t
                    : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

float nk_ease_out_expo(float t) {
    return t >= 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
}

float nk_ease_out_elastic(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return powf(2.0f, -10.0f * t) * sinf((t - 0.075f) * (2.0f * (float)M_PI) / 0.3f) + 1.0f;
}

float nk_ease_out_bounce(float t) {
    if (t < 1.0f / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2.0f / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}

// ── Cubic bezier easing ──

static float bezier_sample(float p1, float p2, float t) {
    // B(t) = 3*(1-t)^2*t*p1 + 3*(1-t)*t^2*p2 + t^3
    float it = 1.0f - t;
    return 3.0f * it * it * t * p1 + 3.0f * it * t * t * p2 + t * t * t;
}

static float bezier_slope(float p1, float p2, float t) {
    float it = 1.0f - t;
    return 3.0f * it * it * p1 + 6.0f * it * t * (p2 - p1) + 3.0f * t * t * (1.0f - p2);
}

float nk_ease_cubic_bezier(float p1x, float p1y, float p2x, float p2y, float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    // Newton-Raphson to find parameter for x=t
    float guess = t;
    for (int i = 0; i < 8; i++) {
        float x = bezier_sample(p1x, p2x, guess) - t;
        float dx = bezier_slope(p1x, p2x, guess);
        if (fabsf(dx) < 1e-7f) break;
        guess -= x / dx;
    }
    return bezier_sample(p1y, p2y, guess);
}

// Material Design 3 curves (approximated as cubic-bezier)
float nk_ease_md3_standard(float t) {
    return nk_ease_cubic_bezier(0.2f, 0.0f, 0.0f, 1.0f, t);
}

float nk_ease_md3_emphasized(float t) {
    // MD3 emphasized: two-segment curve approximated
    if (t < 0.5f) {
        return nk_ease_cubic_bezier(0.05f, 0.0f, 0.133333f, 0.06f, t * 2.0f) * 0.5f;
    } else {
        return 0.5f + nk_ease_cubic_bezier(0.208333f, 0.82f, 0.25f, 1.0f, (t - 0.5f) * 2.0f) * 0.5f;
    }
}

