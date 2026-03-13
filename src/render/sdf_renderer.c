/**
 * @file sdf_renderer.c
 * @brief Batched SDF renderer — flushes quads via push constants.
 */

#include "sdf_renderer.h"
#include "../core/nk_log.h"

#include <string.h>

NkResult nk_sdf_init(NkSdfRenderer *sdf, NkVkContext *vk, VkFormat color_format) {
    memset(sdf, 0, sizeof(*sdf));
    return nk_pipeline_create_sdf(vk, color_format, &sdf->pipeline);
}

void nk_sdf_begin(NkSdfRenderer *sdf, VkExtent2D extent) {
    sdf->batch_count     = 0;
    sdf->viewport_extent = extent;
}

// Cached transform for current viewport — avoids division per-quad
static inline void get_transform(const NkSdfRenderer *sdf, float out[4]) {
    out[0] = -1.0f;
    out[1] = -1.0f;
    out[2] = 2.0f / (float)sdf->viewport_extent.width;
    out[3] = 2.0f / (float)sdf->viewport_extent.height;
}

void nk_sdf_rect(NkSdfRenderer *sdf, NkRect rect, NkCorners corners,
                  NkColor fill, float border_width, float softness) {
    if (sdf->batch_count >= NK_SDF_MAX_QUADS) return;

    NkSdfQuad *q = &sdf->batch[sdf->batch_count++];

    float t[4];
    get_transform(sdf, t);

    q->push = (NkSdfPushConstants){
        .transform = { t[0], t[1], t[2], t[3] },
        .rect      = { rect.x, rect.y, rect.w, rect.h },
        .color     = { fill.r, fill.g, fill.b, fill.a },
        .corners   = { corners.top_left, corners.top_right,
                       corners.bottom_right, corners.bottom_left },
        .params    = { border_width, softness, 0.0f, 0.0f },
    };
}

void nk_sdf_circle(NkSdfRenderer *sdf, NkVec2 center, float radius, NkColor fill) {
    NkRect rect = { center.x - radius, center.y - radius, radius * 2, radius * 2 };
    NkCorners corners = { radius, radius, radius, radius };
    nk_sdf_rect(sdf, rect, corners, fill, 0.0f, 1.0f);
}

void nk_sdf_morph(NkSdfRenderer *sdf, NkVec2 center, float radius,
                   float morph_t, float corner_radius,
                   NkColor fill, float border_width, float softness) {
    if (sdf->batch_count >= NK_SDF_MAX_QUADS) return;

    NkSdfQuad *q = &sdf->batch[sdf->batch_count++];

    float size = radius * 2.0f;
    float t[4];
    get_transform(sdf, t);

    // morph_t must be > 0.001 to activate morph mode in shader
    float mt = morph_t < 0.002f ? 0.002f : morph_t;

    q->push = (NkSdfPushConstants){
        .transform = { t[0], t[1], t[2], t[3] },
        .rect      = { center.x - radius, center.y - radius, size, size },
        .color     = { fill.r, fill.g, fill.b, fill.a },
        .corners   = { 0, 0, 0, 0 },
        .params    = { border_width, softness > 0 ? softness : 1.0f, mt, corner_radius },
    };
}

void nk_sdf_flush(NkSdfRenderer *sdf, VkCommandBuffer cmd) {
    if (sdf->batch_count == 0) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sdf->pipeline.pipeline);

    VkViewport viewport = {
        .x = 0, .y = 0,
        .width  = (float)sdf->viewport_extent.width,
        .height = (float)sdf->viewport_extent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f,
    };
    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = sdf->viewport_extent,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (uint32_t i = 0; i < sdf->batch_count; i++) {
        vkCmdPushConstants(cmd, sdf->pipeline.layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(NkSdfPushConstants), &sdf->batch[i].push);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }

    sdf->batch_count = 0;
}

void nk_sdf_destroy(NkSdfRenderer *sdf, NkVkContext *vk) {
    nk_pipeline_destroy(vk, &sdf->pipeline);
    memset(sdf, 0, sizeof(*sdf));
}
