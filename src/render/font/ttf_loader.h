/**
 * @file ttf_loader.h
 * @brief TTF/OTF file loading and font metric extraction via stb_truetype.
 */

#ifndef NK_TTF_LOADER_H
#define NK_TTF_LOADER_H

#include <nykhara/types.h>
#include <stdint.h>

// Forward declare stbtt_fontinfo to avoid leaking stb_truetype internals
typedef struct stbtt_fontinfo stbtt_fontinfo;

/**
 * Loaded TTF font data handle.
 */
typedef struct NkTtfFont {
    uint8_t        *ttf_data;   // Raw TTF file bytes (heap-allocated)
    stbtt_fontinfo *font_info;  // stb_truetype font info (heap-allocated)
    float           scale;      // Pixel scale for the requested font size
    float           ascent;     // Scaled ascent
    float           descent;    // Scaled descent
    float           line_gap;   // Scaled line gap
} NkTtfFont;

/**
 * Load a TTF/OTF file, initialize stb_truetype, and compute metrics.
 */
NkResult nk_ttf_load(NkTtfFont *font, const char *ttf_path, float font_size);

/**
 * Get horizontal advance for a codepoint.
 */
float nk_ttf_get_advance(const NkTtfFont *font, uint32_t codepoint);

/**
 * Free font data.
 */
void nk_ttf_destroy(NkTtfFont *font);

#endif // NK_TTF_LOADER_H
