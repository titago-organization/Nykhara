/**
 * @file vk_swapchain.h
 * @brief Vulkan swapchain creation, recreation, and present.
 */

#ifndef NK_VK_SWAPCHAIN_H
#define NK_VK_SWAPCHAIN_H

#include "vk_init.h"

#define NK_MAX_FRAMES_IN_FLIGHT 2

typedef struct NkSwapchain {
    VkSwapchainKHR   swapchain;
    VkSurfaceKHR     surface;
    VkFormat          format;
    VkExtent2D        extent;
    VkPresentModeKHR  present_mode;
    uint32_t          image_count;
    VkImage          *images;
    VkImageView      *views;
} NkSwapchain;

NkResult nk_swapchain_create(NkVkContext *vk, VkSurfaceKHR surface,
                              uint32_t width, uint32_t height,
                              NkSwapchain *out);
NkResult nk_swapchain_recreate(NkVkContext *vk, NkSwapchain *sc,
                                uint32_t width, uint32_t height);
void     nk_swapchain_destroy(NkVkContext *vk, NkSwapchain *sc);

#endif // NK_VK_SWAPCHAIN_H

