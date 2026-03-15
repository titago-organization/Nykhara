/**
 * @file vk_cmd.c
 * @brief Per-frame command buffer recording, submission, and presentation.
 *
 * Semaphore strategy:
 * - acquire_sems[N+1]: round-robin, picked BEFORE vkAcquireNextImage.
 *   We need N+1 because at most N images can be in-flight, plus one for the
 *   current acquire call.
 * - present_sems[N]: indexed by the acquired image index.
 *   present_sems[i] is signaled by vkQueueSubmit when rendering image i,
 *   and waited by vkQueuePresent. It is safe to reuse because by the time
 *   we render image i again, the previous present of image i has completed.
 *
 * See: https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
 */

#include "vk_cmd.h"
#include "../core/nk_log.h"

#include <string.h>

// ── Frame sync (command pool + fence) ──

NkResult nk_frame_sync_create(NkVkContext *vk, NkFrameSync *frame) {
    memset(frame, 0, sizeof(*frame));

    VkCommandPoolCreateInfo pool_ci = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vk->graphics_family,
    };
    VkResult vr = vkCreateCommandPool(vk->device, &pool_ci, NULL, &frame->cmd_pool);
    if (vr != VK_SUCCESS) return NK_ERROR_VULKAN;

    VkCommandBufferAllocateInfo alloc_ci = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = frame->cmd_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    vr = vkAllocateCommandBuffers(vk->device, &alloc_ci, &frame->cmd_buf);
    if (vr != VK_SUCCESS) return NK_ERROR_VULKAN;

    VkFenceCreateInfo fence_ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    vkCreateFence(vk->device, &fence_ci, NULL, &frame->in_flight);

    return NK_SUCCESS;
}

void nk_frame_sync_destroy(NkVkContext *vk, NkFrameSync *frame) {
    if (frame->in_flight) vkDestroyFence(vk->device, frame->in_flight, NULL);
    if (frame->cmd_pool)  vkDestroyCommandPool(vk->device, frame->cmd_pool, NULL);
    memset(frame, 0, sizeof(*frame));
}

// ── Swapchain sync (semaphore pools) ──

NkResult nk_swapchain_sync_create(NkVkContext *vk, uint32_t image_count, NkSwapchainSync *sync) {
    memset(sync, 0, sizeof(*sync));

    sync->acquire_count = image_count + 1;
    if (sync->acquire_count > NK_MAX_ACQUIRE_SEMS)
        sync->acquire_count = NK_MAX_ACQUIRE_SEMS;

    sync->present_count = image_count;
    if (sync->present_count > NK_MAX_SWAPCHAIN_IMAGES)
        sync->present_count = NK_MAX_SWAPCHAIN_IMAGES;

    VkSemaphoreCreateInfo sem_ci = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    for (uint32_t i = 0; i < sync->acquire_count; i++) {
        VkResult vr = vkCreateSemaphore(vk->device, &sem_ci, NULL, &sync->acquire_sems[i]);
        if (vr != VK_SUCCESS) return NK_ERROR_VULKAN;
    }
    for (uint32_t i = 0; i < sync->present_count; i++) {
        VkResult vr = vkCreateSemaphore(vk->device, &sem_ci, NULL, &sync->present_sems[i]);
        if (vr != VK_SUCCESS) return NK_ERROR_VULKAN;
    }

    sync->acquire_index = 0;
    return NK_SUCCESS;
}

void nk_swapchain_sync_destroy(NkVkContext *vk, NkSwapchainSync *sync) {
    for (uint32_t i = 0; i < sync->acquire_count; i++)
        if (sync->acquire_sems[i]) vkDestroySemaphore(vk->device, sync->acquire_sems[i], NULL);
    for (uint32_t i = 0; i < sync->present_count; i++)
        if (sync->present_sems[i]) vkDestroySemaphore(vk->device, sync->present_sems[i], NULL);
    memset(sync, 0, sizeof(*sync));
}

// ── Frame begin/end ──

NkResult nk_frame_begin(NkVkContext *vk, NkSwapchain *sc,
                         NkFrameSync *frame, NkSwapchainSync *sync,
                         uint32_t *out_image) {
    // Wait for this frame slot's previous submission
    vkWaitForFences(vk->device, 1, &frame->in_flight, VK_TRUE, UINT64_MAX);

    // Pick acquire semaphore round-robin (we don't know which image yet)
    VkSemaphore acquire_sem = sync->acquire_sems[sync->acquire_index % sync->acquire_count];
    sync->acquire_index++;

    VkResult vr = vkAcquireNextImageKHR(vk->device, sc->swapchain, UINT64_MAX,
                                         acquire_sem, VK_NULL_HANDLE, out_image);
    if (vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR) {
        return NK_ERROR_VULKAN;
    }

    // Now that acquiring succeeded, we commit to the frame and reset the fence
    vkResetFences(vk->device, 1, &frame->in_flight);

    // Store the acquire semaphore for frame_end to wait on
    sync->_last_acquire_sem = acquire_sem;

    vkResetCommandBuffer(frame->cmd_buf, 0);

    VkCommandBufferBeginInfo begin_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(frame->cmd_buf, &begin_ci);

    return NK_SUCCESS;
}

NkResult nk_frame_end(NkVkContext *vk, NkSwapchain *sc,
                       NkFrameSync *frame, NkSwapchainSync *sync,
                       uint32_t image_index) {
    vkEndCommandBuffer(frame->cmd_buf);

    // Wait on the acquire semaphore that was used for THIS image
    VkSemaphore wait_sem = sync->_last_acquire_sem;
    // Signal the present semaphore for THIS image index
    VkSemaphore signal_sem = sync->present_sems[image_index];

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &wait_sem,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &frame->cmd_buf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &signal_sem,
    };

    VkResult vr = vkQueueSubmit(vk->graphics_queue, 1, &submit, frame->in_flight);
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkQueueSubmit failed: %d", vr);
        return NK_ERROR_VULKAN;
    }

    VkPresentInfoKHR present = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &signal_sem,
        .swapchainCount     = 1,
        .pSwapchains        = &sc->swapchain,
        .pImageIndices      = &image_index,
    };

    vr = vkQueuePresentKHR(vk->present_queue, &present);
    if (vr == VK_ERROR_OUT_OF_DATE_KHR || vr == VK_SUBOPTIMAL_KHR) {
        return NK_ERROR_VULKAN;
    }

    return NK_SUCCESS;
}
