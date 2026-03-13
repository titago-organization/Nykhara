/**
 * @file font_atlas.c
 * @brief SDF font atlas — TTF → stb_truetype SDF → stb_rect_pack → atlas bitmap.
 */

#include "font_atlas.h"
#include "../../core/nk_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// stb_rect_pack MUST be included BEFORE stb_truetype to avoid type conflicts.
// stb_truetype has internal stb_rect_pack shims; the real impl must come first.
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

// Range: ASCII printable (32-126) — extend later for Unicode blocks
#define FIRST_CHAR  32
#define LAST_CHAR   126
#define CHAR_COUNT  (LAST_CHAR - FIRST_CHAR + 1)

NkResult nk_font_atlas_build(NkFontAtlas *atlas, const char *ttf_path, float font_size) {
    memset(atlas, 0, sizeof(*atlas));

    // Read TTF file
    FILE *f = fopen(ttf_path, "rb");
    if (!f) {
        NK_LOG_ERROR("Cannot open font: %s", ttf_path);
        return NK_ERROR_IO;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *ttf_data = malloc((size_t)fsize);
    fread(ttf_data, 1, (size_t)fsize, f);
    fclose(f);

    // Init stb_truetype
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, ttf_data, stbtt_GetFontOffsetForIndex(ttf_data, 0))) {
        NK_LOG_ERROR("stbtt_InitFont failed for %s", ttf_path);
        free(ttf_data);
        return NK_ERROR_IO;
    }

    float scale = stbtt_ScaleForPixelHeight(&font, font_size);

    int ascent_i, descent_i, line_gap_i;
    stbtt_GetFontVMetrics(&font, &ascent_i, &descent_i, &line_gap_i);
    atlas->ascent   = (float)ascent_i * scale;
    atlas->descent  = (float)descent_i * scale;
    atlas->line_gap = (float)line_gap_i * scale;
    atlas->font_size = font_size;

    // Allocate atlas bitmap
    atlas->atlas_size = NK_FONT_ATLAS_SIZE;
    atlas->bitmap = calloc(atlas->atlas_size * atlas->atlas_size, 1);

    // Pack glyphs using stb_rect_pack
    stbrp_context rp_ctx;
    stbrp_node *rp_nodes = calloc(atlas->atlas_size, sizeof(stbrp_node));
    stbrp_init_target(&rp_ctx, (int)atlas->atlas_size, (int)atlas->atlas_size,
                      rp_nodes, (int)atlas->atlas_size);

    atlas->glyph_count = CHAR_COUNT;
    atlas->glyphs = calloc(CHAR_COUNT, sizeof(NkGlyphInfo));

    // Rasterize SDF for each glyph and pack into atlas
    for (uint32_t i = 0; i < CHAR_COUNT; i++) {
        uint32_t cp = FIRST_CHAR + i;
        int w, h, xoff, yoff;

        uint8_t *sdf = stbtt_GetCodepointSDF(&font, scale, (int)cp,
                                               NK_FONT_SDF_PADDING,
                                               NK_FONT_SDF_ONEDGE,
                                               NK_FONT_SDF_SCALE,
                                               &w, &h, &xoff, &yoff);

        atlas->glyphs[i].codepoint = cp;
        atlas->glyphs[i].xoff = (float)xoff;
        atlas->glyphs[i].yoff = (float)yoff;

        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, (int)cp, &advance, &lsb);
        atlas->glyphs[i].xadvance = (float)advance * scale;

        if (!sdf || w == 0 || h == 0) {
            atlas->glyphs[i].packed = 0;
            if (sdf) stbtt_FreeSDF(sdf, NULL);
            continue;
        }

        // Pack rect
        stbrp_rect rc = { .id = (int)i, .w = (stbrp_coord)w, .h = (stbrp_coord)h };
        stbrp_pack_rects(&rp_ctx, &rc, 1);

        if (rc.was_packed) {
            // Copy SDF bitmap into atlas
            for (int row = 0; row < h; row++) {
                memcpy(atlas->bitmap + (rc.y + row) * atlas->atlas_size + rc.x,
                       sdf + row * w, (size_t)w);
            }
            float inv = 1.0f / (float)atlas->atlas_size;
            atlas->glyphs[i].x0 = (float)rc.x * inv;
            atlas->glyphs[i].y0 = (float)rc.y * inv;
            atlas->glyphs[i].x1 = (float)(rc.x + w) * inv;
            atlas->glyphs[i].y1 = (float)(rc.y + h) * inv;
            atlas->glyphs[i].packed = 1;
        }

        stbtt_FreeSDF(sdf, NULL);
    }

    free(rp_nodes);
    free(ttf_data);

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

