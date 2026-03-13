/**
 * @file sdf_renderer.h
 * @brief Batched SDF draw-call builder for rects, text, bezier curves.
 */

#ifndef NK_SDF_RENDERER_H
#define NK_SDF_RENDERER_H

#include "vk_init.h"
#include "vk_pipeline.h"
#include "vk_mem.h"
#include <nykhara/types.h>

#define NK_SDF_MAX_QUADS 512

typedef struct NkSdfQuad {
    NkSdfPushConstants push;
} NkSdfQuad;

typedef struct NkSdfRenderer {
    NkPipeline       pipeline;
    NkSdfQuad        batch[NK_SDF_MAX_QUADS];
    uint32_t         batch_count;
    VkExtent2D       viewport_extent;
} NkSdfRenderer;

/**
 * Initialize the SDF renderer (creates pipeline).
 */
NkResult nk_sdf_init(NkSdfRenderer *sdf, NkVkContext *vk, VkFormat color_format);

/**
 * Begin a new frame batch.
 */
void nk_sdf_begin(NkSdfRenderer *sdf, VkExtent2D extent);

/**
 * Push a rounded rect.
 */
void nk_sdf_rect(NkSdfRenderer *sdf, NkRect rect, NkCorners corners,
                  NkColor fill, float border_width, float softness);

/**
 * Push a circle.
 */
void nk_sdf_circle(NkSdfRenderer *sdf, NkVec2 center, float radius, NkColor fill);


/**
 * Push a shape morph primitive.
 * morph_t: 0.0 = triangle, 1.0 = square, 2.0 = circle.
 * Fractional values give smooth in-between shapes (SDF blending).
 */
void nk_sdf_morph(NkSdfRenderer *sdf, NkVec2 center, float radius,
                   float morph_t, float corner_radius,
                   NkColor fill, float border_width, float softness);

/**
 * Flush all batched primitives into the command buffer.
 */
void nk_sdf_flush(NkSdfRenderer *sdf, VkCommandBuffer cmd);

/**
 * Destroy.
 */
void nk_sdf_destroy(NkSdfRenderer *sdf, NkVkContext *vk);

#endif // NK_SDF_RENDERER_H

