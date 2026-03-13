/**
 * @file vk_cmd.h
 * @brief Command pool/buffer management, per-frame recording.
 *
 * Semaphore strategy (from Vulkan Guide):
 * - image_available semaphores: pool of (image_count+1), picked round-robin BEFORE acquire.
 *   We don't know which image we'll get, so we pre-pick a semaphore.
 * - render_finished semaphores: indexed by acquired image index AFTER acquire.
 *   This ensures the present semaphore for image N is only reused when image N is re-acquired.
 */

#ifndef NK_VK_CMD_H
#define NK_VK_CMD_H

#include "vk_init.h"
#include "vk_swapchain.h"

#define NK_MAX_SWAPCHAIN_IMAGES 8
#define NK_MAX_ACQUIRE_SEMS     (NK_MAX_SWAPCHAIN_IMAGES + 1)

typedef struct NkFrameSync {
    VkCommandPool   cmd_pool;
    VkCommandBuffer cmd_buf;
    VkFence         in_flight;
} NkFrameSync;

/**
 * Synchronization pool for the entire swapchain.
 * acquire_sems:  round-robin pool, one is picked before vkAcquireNextImage.
 * present_sems:  indexed by image index, signaled by submit, waited by present.
 */
typedef struct NkSwapchainSync {
    VkSemaphore acquire_sems[NK_MAX_ACQUIRE_SEMS];
    VkSemaphore present_sems[NK_MAX_SWAPCHAIN_IMAGES];
    uint32_t    acquire_count;
    uint32_t    present_count;
    uint32_t    acquire_index;    // round-robin counter
    VkSemaphore _last_acquire_sem; // set by frame_begin, read by frame_end
} NkSwapchainSync;

NkResult nk_frame_sync_create(NkVkContext *vk, NkFrameSync *frame);
void     nk_frame_sync_destroy(NkVkContext *vk, NkFrameSync *frame);

NkResult nk_swapchain_sync_create(NkVkContext *vk, uint32_t image_count, NkSwapchainSync *sync);
void     nk_swapchain_sync_destroy(NkVkContext *vk, NkSwapchainSync *sync);

/**
 * Begin recording a frame.
 * Picks an acquire semaphore internally (round-robin).
 * Returns NK_SUCCESS or NK_ERROR_VULKAN if swapchain needs recreation.
 */
NkResult nk_frame_begin(NkVkContext *vk, NkSwapchain *sc,
                         NkFrameSync *frame, NkSwapchainSync *sync,
                         uint32_t *out_image);

/**
 * End recording, submit, and present.
 * Uses present_sems[image_index] for the render→present semaphore.
 */
NkResult nk_frame_end(NkVkContext *vk, NkSwapchain *sc,
                       NkFrameSync *frame, NkSwapchainSync *sync,
                       uint32_t image_index);

#endif // NK_VK_CMD_H

