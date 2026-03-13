/**
 * @file font_atlas.h
 * @brief SDF font atlas — stb_truetype rasterization + stb_rect_pack atlas packing.
 */

#ifndef NK_FONT_ATLAS_H
#define NK_FONT_ATLAS_H

#include <nykhara/types.h>
#include <stdint.h>

#define NK_FONT_ATLAS_SIZE 1024   // 1024x1024 R8 texture
#define NK_FONT_SDF_PADDING 6
#define NK_FONT_SDF_ONEDGE  128
#define NK_FONT_SDF_SCALE   32.0f

typedef struct NkGlyphInfo {
    uint32_t codepoint;
    float    x0, y0, x1, y1;   // Texcoords in atlas [0..1]
    float    xoff, yoff;       // Pixel offset from cursor
    float    xadvance;         // Horizontal advance
    int      packed;           // Was this glyph packed?
} NkGlyphInfo;

typedef struct NkFontAtlas {
    uint8_t     *bitmap;           // R8 SDF bitmap (atlas_size x atlas_size)
    uint32_t     atlas_size;
    NkGlyphInfo *glyphs;           // Array of packed glyphs
    uint32_t     glyph_count;
    float        font_size;
    float        ascent;
    float        descent;
    float        line_gap;
} NkFontAtlas;

/**
 * Load a TTF/OTF file and build an SDF atlas.
 * Rasterizes ASCII (32–126) + common Unicode ranges.
 */
NkResult nk_font_atlas_build(NkFontAtlas *atlas, const char *ttf_path, float font_size);

/**
 * Look up glyph info for a codepoint.
 */
const NkGlyphInfo *nk_font_atlas_glyph(const NkFontAtlas *atlas, uint32_t codepoint);

/**
 * Destroy atlas resources.
 */
void nk_font_atlas_destroy(NkFontAtlas *atlas);

#endif // NK_FONT_ATLAS_H

