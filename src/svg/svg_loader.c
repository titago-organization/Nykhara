/**
 * @file svg_loader.c
 * @brief SVG loader via NanoSVG.
 */

#include "svg_loader.h"
#include "../core/nk_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>

NkResult nk_svg_load_file(const char *path, NkSvgImage *out) {
    memset(out, 0, sizeof(*out));

    NSVGimage *image = nsvgParseFromFile(path, "px", 96.0f);
    if (!image) {
        NK_LOG_ERROR("Failed to parse SVG: %s", path);
        return NK_ERROR_IO;
    }

    out->nsvg_image = image;
    out->width      = image->width;
    out->height     = image->height;

    NK_LOG_DEBUG("SVG loaded: %s (%.0fx%.0f)", path, out->width, out->height);
    return NK_SUCCESS;
}

NkResult nk_svg_load_string(const char *svg_data, NkSvgImage *out) {
    memset(out, 0, sizeof(*out));

    // NanoSVG modifies the input string, so we need a copy
    size_t len = strlen(svg_data);
    char *copy = malloc(len + 1);
    if (!copy) return NK_ERROR_OUT_OF_MEMORY;
    memcpy(copy, svg_data, len + 1);

    NSVGimage *image = nsvgParse(copy, "px", 96.0f);
    free(copy);

    if (!image) {
        NK_LOG_ERROR("Failed to parse SVG from string");
        return NK_ERROR_IO;
    }

    out->nsvg_image = image;
    out->width      = image->width;
    out->height     = image->height;
    return NK_SUCCESS;
}

void nk_svg_destroy(NkSvgImage *svg) {
    if (svg->nsvg_image) {
        nsvgDelete(svg->nsvg_image);
    }
    memset(svg, 0, sizeof(*svg));
}

