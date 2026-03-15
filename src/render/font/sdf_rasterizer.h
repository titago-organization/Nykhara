/**
 * @file sdf_rasterizer.h
 * @brief SDF glyph rasterization via stb_truetype.
 */

#ifndef NK_SDF_RASTERIZER_H
#define NK_SDF_RASTERIZER_H

#include <stdint.h>

// Forward declarations
typedef struct stbtt_fontinfo stbtt_fontinfo;

/**
 * Result of rasterizing a single glyph into an SDF bitmap.
 */
typedef struct NkSdfGlyph {
    uint8_t *sdf_bitmap;   // SDF pixel data (caller must free via nk_sdf_free)
    int      width;        // Glyph bitmap width in pixels
    int      height;       // Glyph bitmap height in pixels
    int      xoff;         // Pixel offset from cursor (horizontal)
    int      yoff;         // Pixel offset from cursor (vertical)
} NkSdfGlyph;

/**
 * Rasterize a single codepoint into an SDF bitmap.
 * Returns true if a non-empty glyph was produced.
 */
int nk_sdf_rasterize_glyph(NkSdfGlyph *out,
                            const stbtt_fontinfo *font_info,
                            float scale,
                            uint32_t codepoint,
                            int sdf_padding,
                            uint8_t sdf_onedge,
                            float sdf_scale);

/**
 * Free the SDF bitmap memory returned by nk_sdf_rasterize_glyph.
 */
void nk_sdf_free(NkSdfGlyph *glyph);

#endif // NK_SDF_RASTERIZER_H
