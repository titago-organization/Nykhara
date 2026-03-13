/**
 * @file vk_pipeline.h
 * @brief Vulkan graphics pipeline creation from compiled SPIR-V.
 */

#ifndef NK_VK_PIPELINE_H
#define NK_VK_PIPELINE_H

#include "vk_init.h"

/**
 * Load a SPIR-V file and create a VkShaderModule.
 */
NkResult nk_vk_load_shader(VkDevice device, const char *path, VkShaderModule *out);

/**
 * SDF pipeline layout + push constants for rect/text/bezier rendering.
 */
typedef struct NkSdfPushConstants {
    float transform[4]; // x, y, scale_x, scale_y
    float rect[4];      // x, y, w, h
    float color[4];     // r, g, b, a
    float corners[4];   // corner radii (top-left, top-right, bottom-right, bottom-left)
    float params[4];    // extra: border_width, softness, ...
} NkSdfPushConstants;

typedef struct NkPipeline {
    VkPipeline       pipeline;
    VkPipelineLayout layout;
} NkPipeline;

/**
 * Create the SDF rendering pipeline (rounded rects, text, bezier).
 */
NkResult nk_pipeline_create_sdf(NkVkContext *vk, VkFormat color_format, NkPipeline *out);

/**
 * Destroy a pipeline.
 */
void nk_pipeline_destroy(NkVkContext *vk, NkPipeline *p);

#endif // NK_VK_PIPELINE_H

