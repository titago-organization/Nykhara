/**
 * @file sdf_rasterizer.c
 * @brief SDF glyph rasterization via stb_truetype.
 *
 * This file does NOT own STB_TRUETYPE_IMPLEMENTATION — that belongs to
 * ttf_loader.c.  We only use the stbtt_* API here.
 */

#include "sdf_rasterizer.h"

#include <stb_rect_pack.h>
#include <stb_truetype.h>
#include <string.h>

int nk_sdf_rasterize_glyph(NkSdfGlyph *out,
                            const stbtt_fontinfo *font_info,
                            float scale,
                            uint32_t codepoint,
                            int sdf_padding,
                            uint8_t sdf_onedge,
                            float sdf_scale) {
    memset(out, 0, sizeof(*out));

    int w, h, xoff, yoff;
    uint8_t *sdf = stbtt_GetCodepointSDF(font_info, scale, (int)codepoint,
                                          sdf_padding, sdf_onedge, sdf_scale,
                                          &w, &h, &xoff, &yoff);

    if (!sdf || w == 0 || h == 0) {
        if (sdf) stbtt_FreeSDF(sdf, NULL);
        return 0;
    }

    out->sdf_bitmap = sdf;
    out->width      = w;
    out->height     = h;
    out->xoff       = xoff;
    out->yoff       = yoff;
    return 1;
}

void nk_sdf_free(NkSdfGlyph *glyph) {
    if (glyph->sdf_bitmap) {
        stbtt_FreeSDF(glyph->sdf_bitmap, NULL);
        glyph->sdf_bitmap = NULL;
    }
}
