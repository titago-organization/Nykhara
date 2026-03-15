/**
 * @file ttf_loader.c
 * @brief TTF/OTF file loading and font metric extraction via stb_truetype.
 *
 * This file owns the STB_TRUETYPE_IMPLEMENTATION and the prerequisite
 * STB_RECT_PACK_IMPLEMENTATION (stb_truetype requires stb_rect_pack types
 * to be defined first when used in the same TU).
 */

#include "ttf_loader.h"
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

NkResult nk_ttf_load(NkTtfFont *font, const char *ttf_path, float font_size) {
    memset(font, 0, sizeof(*font));

    // Read TTF file
    FILE *f = fopen(ttf_path, "rb");
    if (!f) {
        NK_LOG_ERROR("Cannot open font: %s", ttf_path);
        return NK_ERROR_IO;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    font->ttf_data = malloc((size_t)fsize);
    if (!font->ttf_data) {
        fclose(f);
        return NK_ERROR_OUT_OF_MEMORY;
    }
    fread(font->ttf_data, 1, (size_t)fsize, f);
    fclose(f);

    // Allocate and init stb_truetype font info
    font->font_info = malloc(sizeof(stbtt_fontinfo));
    if (!font->font_info) {
        free(font->ttf_data);
        font->ttf_data = NULL;
        return NK_ERROR_OUT_OF_MEMORY;
    }

    if (!stbtt_InitFont(font->font_info, font->ttf_data,
                        stbtt_GetFontOffsetForIndex(font->ttf_data, 0))) {
        NK_LOG_ERROR("stbtt_InitFont failed for %s", ttf_path);
        free(font->font_info);
        free(font->ttf_data);
        font->font_info = NULL;
        font->ttf_data = NULL;
        return NK_ERROR_IO;
    }

    font->scale = stbtt_ScaleForPixelHeight(font->font_info, font_size);

    int ascent_i, descent_i, line_gap_i;
    stbtt_GetFontVMetrics(font->font_info, &ascent_i, &descent_i, &line_gap_i);
    font->ascent   = (float)ascent_i * font->scale;
    font->descent  = (float)descent_i * font->scale;
    font->line_gap = (float)line_gap_i * font->scale;

    return NK_SUCCESS;
}

float nk_ttf_get_advance(const NkTtfFont *font, uint32_t codepoint) {
    int advance, lsb;
    stbtt_GetCodepointHMetrics(font->font_info, (int)codepoint, &advance, &lsb);
    return (float)advance * font->scale;
}

void nk_ttf_destroy(NkTtfFont *font) {
    free(font->font_info);
    free(font->ttf_data);
    memset(font, 0, sizeof(*font));
}
