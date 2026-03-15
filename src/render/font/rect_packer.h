/**
 * @file rect_packer.h
 * @brief Atlas rectangle packing via stb_rect_pack.
 */

#ifndef NK_RECT_PACKER_H
#define NK_RECT_PACKER_H

#include <nykhara/types.h>
#include <stdint.h>

/**
 * Rectangle packer context wrapping stb_rect_pack.
 */
typedef struct NkRectPacker {
    void     *rp_ctx;     // stbrp_context*  (opaque)
    void     *rp_nodes;   // stbrp_node[]    (opaque, heap-allocated)
    uint32_t  atlas_size; // Width/height of the atlas
} NkRectPacker;

/**
 * Packed rectangle result.
 */
typedef struct NkPackedRect {
    int x, y;       // Top-left position in the atlas
    int was_packed;  // True if packing succeeded
} NkPackedRect;

/**
 * Initialize the packer for a given atlas size.
 */
NkResult nk_rect_packer_init(NkRectPacker *packer, uint32_t atlas_size);

/**
 * Pack a single rectangle of the given width/height.
 */
NkPackedRect nk_rect_packer_pack(NkRectPacker *packer, int w, int h);

/**
 * Destroy the packer and free internal resources.
 */
void nk_rect_packer_destroy(NkRectPacker *packer);

#endif // NK_RECT_PACKER_H
