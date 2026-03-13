/**
 * @file types.h
 * @brief Fundamental types for the Nykhara GUI framework.
 */

#ifndef NK_TYPES_H
#define NK_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ── Result codes ──

typedef enum NkResult {
    NK_SUCCESS              =  0,
    NK_ERROR_UNKNOWN        = -1,
    NK_ERROR_OUT_OF_MEMORY  = -2,
    NK_ERROR_INIT_FAILED    = -3,
    NK_ERROR_VULKAN         = -4,
    NK_ERROR_WAYLAND        = -5,
    NK_ERROR_LUA            = -6,
    NK_ERROR_PLUGIN         = -7,
    NK_ERROR_IO             = -8,
    NK_ERROR_INVALID_ARG    = -9,
} NkResult;

// ── Handles (opaque pointers, type-safe) ──

typedef struct NkWidget     NkWidget;
typedef struct NkNode       NkNode;
typedef struct NkApp        NkApp;
typedef struct NkPlugin     NkPlugin;
typedef struct NkAnimation  NkAnimation;

// ── Geometry ──

typedef struct NkVec2 {
    float x, y;
} NkVec2;

typedef struct NkVec4 {
    float x, y, z, w;
} NkVec4;

typedef struct NkRect {
    float x, y, w, h;
} NkRect;

// ── Color (RGBA, 0.0–1.0) ──

typedef struct NkColor {
    float r, g, b, a;
} NkColor;

static inline NkColor nk_color_rgba(float r, float g, float b, float a) {
    return (NkColor){ r, g, b, a };
}

static inline NkColor nk_color_hex(uint32_t hex) {
    return (NkColor){
        .r = (float)((hex >> 24) & 0xFF) / 255.0f,
        .g = (float)((hex >> 16) & 0xFF) / 255.0f,
        .b = (float)((hex >>  8) & 0xFF) / 255.0f,
        .a = (float)((hex >>  0) & 0xFF) / 255.0f,
    };
}

// ── Corners (for rounded rects) ──

typedef struct NkCorners {
    float top_left;
    float top_right;
    float bottom_right;
    float bottom_left;
} NkCorners;

// ── Edge insets (margin / padding) ──

typedef struct NkEdgeInsets {
    float top, right, bottom, left;
} NkEdgeInsets;

// ── Handle type for resource IDs ──

typedef uint64_t NkHandle;
#define NK_INVALID_HANDLE ((NkHandle)0)

#endif // NK_TYPES_H

