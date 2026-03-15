/**
 * @file rect_packer.c
 * @brief Atlas rectangle packing via stb_rect_pack.
 *
 * This file does NOT own STB_RECT_PACK_IMPLEMENTATION — that belongs to
 * ttf_loader.c.  We only use the stbrp_* API here.
 */

#include "rect_packer.h"
#include "../../core/nk_log.h"

#include <stb_rect_pack.h>
#include <stdlib.h>
#include <string.h>

NkResult nk_rect_packer_init(NkRectPacker *packer, uint32_t atlas_size) {
    memset(packer, 0, sizeof(*packer));
    packer->atlas_size = atlas_size;

    stbrp_context *ctx = calloc(1, sizeof(stbrp_context));
    if (!ctx) return NK_ERROR_OUT_OF_MEMORY;

    stbrp_node *nodes = calloc(atlas_size, sizeof(stbrp_node));
    if (!nodes) {
        free(ctx);
        return NK_ERROR_OUT_OF_MEMORY;
    }

    stbrp_init_target(ctx, (int)atlas_size, (int)atlas_size, nodes, (int)atlas_size);

    packer->rp_ctx   = ctx;
    packer->rp_nodes = nodes;
    return NK_SUCCESS;
}

NkPackedRect nk_rect_packer_pack(NkRectPacker *packer, int w, int h) {
    NkPackedRect result = {0};

    stbrp_rect rc = { .id = 0, .w = (stbrp_coord)w, .h = (stbrp_coord)h };
    stbrp_pack_rects((stbrp_context *)packer->rp_ctx, &rc, 1);

    result.x = rc.x;
    result.y = rc.y;
    result.was_packed = rc.was_packed;
    return result;
}

void nk_rect_packer_destroy(NkRectPacker *packer) {
    free(packer->rp_nodes);
    free(packer->rp_ctx);
    memset(packer, 0, sizeof(*packer));
}
