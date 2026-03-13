/**
 * @file morph.c
 * @brief Shape morphing implementation — polygon generation, resampling, lerp.
 */

#include "morph.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void nk_morph_polygon(NkVec2 center, float radius, uint32_t sides, NkMorphShape *out) {
    if (sides > NK_MORPH_MAX_VERTICES) sides = NK_MORPH_MAX_VERTICES;
    out->count = sides;
    for (uint32_t i = 0; i < sides; i++) {
        float angle = 2.0f * (float)M_PI * (float)i / (float)sides - (float)M_PI / 2.0f;
        out->vertices[i].x = center.x + radius * cosf(angle);
        out->vertices[i].y = center.y + radius * sinf(angle);
    }
}

void nk_morph_circle(NkVec2 center, float radius, NkMorphShape *out) {
    nk_morph_polygon(center, radius, 64, out);
}

void nk_morph_rounded_rect(NkRect rect, float cr, NkMorphShape *out) {
    // Generate a rounded rect as a polygon with ~64 vertices
    // 4 arcs of ~12 segments each + 4 straight edges
    uint32_t arc_segs = 12;
    uint32_t idx = 0;

    float x0 = rect.x, y0 = rect.y;
    float x1 = rect.x + rect.w, y1 = rect.y + rect.h;

    // Corners: top-right, bottom-right, bottom-left, top-left
    NkVec2 corners[4] = {
        { x1 - cr, y0 + cr },  // top-right
        { x1 - cr, y1 - cr },  // bottom-right
        { x0 + cr, y1 - cr },  // bottom-left
        { x0 + cr, y0 + cr },  // top-left
    };
    float start_angles[4] = {
        -(float)M_PI / 2.0f,  // top-right: -90°
        0.0f,                   // bottom-right: 0°
        (float)M_PI / 2.0f,   // bottom-left: 90°
        (float)M_PI,           // top-left: 180°
    };

    for (uint32_t c = 0; c < 4; c++) {
        for (uint32_t s = 0; s <= arc_segs; s++) {
            if (idx >= NK_MORPH_MAX_VERTICES) break;
            float t = (float)s / (float)arc_segs;
            float angle = start_angles[c] + t * (float)M_PI / 2.0f;
            out->vertices[idx].x = corners[c].x + cr * cosf(angle);
            out->vertices[idx].y = corners[c].y + cr * sinf(angle);
            idx++;
        }
    }
    out->count = idx;
}

// Compute total perimeter length of a shape
static float shape_perimeter(const NkMorphShape *s) {
    float total = 0;
    for (uint32_t i = 0; i < s->count; i++) {
        uint32_t j = (i + 1) % s->count;
        float dx = s->vertices[j].x - s->vertices[i].x;
        float dy = s->vertices[j].y - s->vertices[i].y;
        total += sqrtf(dx * dx + dy * dy);
    }
    return total;
}

void nk_morph_resample(const NkMorphShape *src, uint32_t target_count, NkMorphShape *out) {
    if (target_count > NK_MORPH_MAX_VERTICES) target_count = NK_MORPH_MAX_VERTICES;
    if (src->count == 0) { out->count = 0; return; }
    if (src->count == target_count) { *out = *src; return; }

    float total = shape_perimeter(src);
    float segment = total / (float)target_count;

    out->count = target_count;
    out->vertices[0] = src->vertices[0];

    uint32_t src_i = 0;
    float src_accum = 0;

    for (uint32_t i = 1; i < target_count; i++) {
        float target_dist = segment * (float)i;

        while (src_i < src->count) {
            uint32_t next = (src_i + 1) % src->count;
            float dx = src->vertices[next].x - src->vertices[src_i].x;
            float dy = src->vertices[next].y - src->vertices[src_i].y;
            float edge_len = sqrtf(dx * dx + dy * dy);

            if (src_accum + edge_len >= target_dist) {
                float t = (target_dist - src_accum) / edge_len;
                out->vertices[i].x = src->vertices[src_i].x + dx * t;
                out->vertices[i].y = src->vertices[src_i].y + dy * t;
                break;
            }
            src_accum += edge_len;
            src_i = next;
            if (src_i == 0) break; // Wrapped around
        }
    }
}

void nk_morph_lerp(const NkMorphShape *a, const NkMorphShape *b, float t, NkMorphShape *out) {
    uint32_t count = a->count < b->count ? a->count : b->count;
    out->count = count;
    for (uint32_t i = 0; i < count; i++) {
        out->vertices[i].x = a->vertices[i].x + (b->vertices[i].x - a->vertices[i].x) * t;
        out->vertices[i].y = a->vertices[i].y + (b->vertices[i].y - a->vertices[i].y) * t;
    }
}

