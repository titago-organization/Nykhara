/**
 * @file svg_loader.h
 * @brief SVG parsing via NanoSVG — load SVG into internal path representation.
 */

#ifndef NK_SVG_LOADER_H
#define NK_SVG_LOADER_H

#include <nykhara/types.h>

typedef struct NkSvgImage {
    void  *nsvg_image;    // NSVGimage* (opaque)
    float  width;
    float  height;
} NkSvgImage;

/**
 * Load an SVG file and parse it.
 */
NkResult nk_svg_load_file(const char *path, NkSvgImage *out);

/**
 * Load SVG from a string buffer.
 */
NkResult nk_svg_load_string(const char *svg_data, NkSvgImage *out);

/**
 * Destroy the parsed SVG.
 */
void nk_svg_destroy(NkSvgImage *svg);

#endif // NK_SVG_LOADER_H

