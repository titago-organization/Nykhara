/**
 * @file text_render.h
 * @brief SDF text rendering helpers shared by widget draw functions.
 */

#ifndef NK_TEXT_RENDER_H
#define NK_TEXT_RENDER_H

#include "../render/sdf_renderer.h"
#include <nykhara/types.h>

/**
 * Draw a text string as SDF rect quads using the built-in 5×7 bitmap font.
 *
 * @param sdf   Active SDF renderer.
 * @param x     Left edge, in pixels.
 * @param y     Top edge, in pixels.
 * @param px    Pixel size per cell (0 ⇒ default 2.0).
 * @param text  Null-terminated ASCII string.
 * @param color Fill colour.
 */
void nk_text_draw(NkSdfRenderer *sdf, float x, float y, float px,
                  const char *text, NkColor color);

#endif /* NK_TEXT_RENDER_H */
