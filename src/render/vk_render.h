/**
 * @file vk_render.h
 * @brief Frame rendering helpers — dynamic rendering, image transitions.
 */

#ifndef NK_VK_RENDER_H
#define NK_VK_RENDER_H

#include "vk_init.h"
#include "vk_swapchain.h"
#include "vk_cmd.h"

/**
 * Transition a swapchain image to color attachment and begin dynamic rendering.
 */
void nk_render_begin(NkVkContext *vk, NkSwapchain *sc, VkCommandBuffer cmd,
                     uint32_t image_index, float clear_r, float clear_g, float clear_b);

/**
 * End dynamic rendering and transition image to present layout.
 */
void nk_render_end(NkVkContext *vk, NkSwapchain *sc, VkCommandBuffer cmd, uint32_t image_index);

#endif // NK_VK_RENDER_H

