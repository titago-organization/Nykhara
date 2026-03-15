/**
 * @file font_atlas.c
 * @brief SDF font atlas orchestrator — loads TTF, rasterizes SDF glyphs,
 *        packs them into a single R8 bitmap atlas.
 *
 * The heavy lifting is delegated to:
 *   - ttf_loader   : TTF file I/O and font metric extraction
 *   - sdf_rasterizer : per-glyph SDF rasterization
 *   - rect_packer  : atlas rectangle bin-packing
 */

#include "font_atlas.h"
#include "ttf_loader.h"
#include "sdf_rasterizer.h"
#include "rect_packer.h"
#include "../../core/nk_log.h"

#include <stdlib.h>
#include <string.h>

// Range: ASCII printable (32-126) — extend later for Unicode blocks
#define FIRST_CHAR  32
#define LAST_CHAR   126
#define CHAR_COUNT  (LAST_CHAR - FIRST_CHAR + 1)

NkResult nk_font_atlas_build(NkFontAtlas *atlas, const char *ttf_path, float font_size) {
    memset(atlas, 0, sizeof(*atlas));

    // Step 1: Load TTF and extract font metrics
    NkTtfFont ttf;
    NkResult r = nk_ttf_load(&ttf, ttf_path, font_size);
    if (r != NK_SUCCESS) return r;

    atlas->ascent    = ttf.ascent;
    atlas->descent   = ttf.descent;
    atlas->line_gap  = ttf.line_gap;
    atlas->font_size = font_size;

    // Allocate atlas bitmap
    atlas->atlas_size = NK_FONT_ATLAS_SIZE;
    atlas->bitmap = calloc(atlas->atlas_size * atlas->atlas_size, 1);
    if (!atlas->bitmap) {
        nk_ttf_destroy(&ttf);
        return NK_ERROR_OUT_OF_MEMORY;
    }

    // Step 2: Initialize rectangle packer
    NkRectPacker packer;
    r = nk_rect_packer_init(&packer, atlas->atlas_size);
    if (r != NK_SUCCESS) {
        free(atlas->bitmap);
        nk_ttf_destroy(&ttf);
        return r;
    }

    atlas->glyph_count = CHAR_COUNT;
    atlas->glyphs = calloc(CHAR_COUNT, sizeof(NkGlyphInfo));
    if (!atlas->glyphs) {
        nk_rect_packer_destroy(&packer);
        free(atlas->bitmap);
        nk_ttf_destroy(&ttf);
        return NK_ERROR_OUT_OF_MEMORY;
    }

    // Step 3: Rasterize SDF for each glyph and pack into atlas
    for (uint32_t i = 0; i < CHAR_COUNT; i++) {
        uint32_t cp = FIRST_CHAR + i;

        atlas->glyphs[i].codepoint = cp;
        atlas->glyphs[i].xadvance  = nk_ttf_get_advance(&ttf, cp);

        // Rasterize SDF bitmap for this glyph
        NkSdfGlyph sdf;
        if (!nk_sdf_rasterize_glyph(&sdf, ttf.font_info, ttf.scale, cp,
                                     NK_FONT_SDF_PADDING,
                                     NK_FONT_SDF_ONEDGE,
                                     NK_FONT_SDF_SCALE)) {
            atlas->glyphs[i].packed = 0;
            continue;
        }

        atlas->glyphs[i].xoff = (float)sdf.xoff;
        atlas->glyphs[i].yoff = (float)sdf.yoff;

        // Pack the glyph rectangle into the atlas
        NkPackedRect rc = nk_rect_packer_pack(&packer, sdf.width, sdf.height);

        if (rc.was_packed) {
            // Copy SDF bitmap into atlas
            for (int row = 0; row < sdf.height; row++) {
                memcpy(atlas->bitmap + (rc.y + row) * atlas->atlas_size + rc.x,
                       sdf.sdf_bitmap + row * sdf.width, (size_t)sdf.width);
            }
            float inv = 1.0f / (float)atlas->atlas_size;
            atlas->glyphs[i].x0 = (float)rc.x * inv;
            atlas->glyphs[i].y0 = (float)rc.y * inv;
            atlas->glyphs[i].x1 = (float)(rc.x + sdf.width) * inv;
            atlas->glyphs[i].y1 = (float)(rc.y + sdf.height) * inv;
            atlas->glyphs[i].packed = 1;
        }

        nk_sdf_free(&sdf);
    }

    nk_rect_packer_destroy(&packer);
    nk_ttf_destroy(&ttf);

    NK_LOG_INFO("Font atlas built: %s, size=%.0f, glyphs=%u, atlas=%ux%u",
                ttf_path, font_size, atlas->glyph_count,
                atlas->atlas_size, atlas->atlas_size);
    return NK_SUCCESS;
}

const NkGlyphInfo *nk_font_atlas_glyph(const NkFontAtlas *atlas, uint32_t codepoint) {
    if (codepoint < FIRST_CHAR || codepoint > LAST_CHAR) return NULL;
    uint32_t idx = codepoint - FIRST_CHAR;
    if (!atlas->glyphs[idx].packed) return NULL;
    return &atlas->glyphs[idx];
}

void nk_font_atlas_destroy(NkFontAtlas *atlas) {
    free(atlas->bitmap);
    free(atlas->glyphs);
    memset(atlas, 0, sizeof(*atlas));
}
