/**
 * @file vk_swapchain.c
 * @brief Swapchain creation and management.
 */

#include "vk_swapchain.h"
#include "../core/nk_log.h"

#include <stdlib.h>
#include <string.h>

static VkSurfaceFormatKHR choose_surface_format(VkPhysicalDevice pd, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &count, NULL);
    VkSurfaceFormatKHR *formats = calloc(count, sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &count, formats);

    VkSurfaceFormatKHR result = formats[0];
    for (uint32_t i = 0; i < count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            result = formats[i];
            break;
        }
    }
    free(formats);
    return result;
}

static VkPresentModeKHR choose_present_mode(VkPhysicalDevice pd, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &count, NULL);
    VkPresentModeKHR *modes = calloc(count, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &count, modes);

    const char *override = getenv("NK_PRESENT_MODE");
    VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;

    bool has_immediate = false;
    bool has_mailbox = false;
    for (uint32_t i = 0; i < count; i++) {
        if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) has_immediate = true;
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) has_mailbox = true;
    }

    if (override) {
        if (strcmp(override, "immediate") == 0 && has_immediate) {
            result = VK_PRESENT_MODE_IMMEDIATE_KHR;
        } else if (strcmp(override, "mailbox") == 0 && has_mailbox) {
            result = VK_PRESENT_MODE_MAILBOX_KHR;
        } else if (strcmp(override, "fifo") == 0) {
            result = VK_PRESENT_MODE_FIFO_KHR;
        }
    } else {
        // Default: IMMEDIATE for maximum FPS, MAILBOX as fallback, FIFO as last resort.
        if (has_immediate) result = VK_PRESENT_MODE_IMMEDIATE_KHR;
        else if (has_mailbox) result = VK_PRESENT_MODE_MAILBOX_KHR;
    }

    free(modes);
    NK_LOG_INFO("Present mode: %s",
        result == VK_PRESENT_MODE_IMMEDIATE_KHR ? "IMMEDIATE (uncapped, tearing possible)" :
        result == VK_PRESENT_MODE_MAILBOX_KHR ? "MAILBOX (vsync, low-latency)" :
                                                  "FIFO (vsync)");
    return result;
}

static NkResult create_image_views(NkVkContext *vk, NkSwapchain *sc) {
    sc->views = calloc(sc->image_count, sizeof(VkImageView));
    for (uint32_t i = 0; i < sc->image_count; i++) {
        VkImageViewCreateInfo ci = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = sc->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = sc->format,
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        VkResult vr = vkCreateImageView(vk->device, &ci, NULL, &sc->views[i]);
        if (vr != VK_SUCCESS) {
            NK_LOG_ERROR("Failed to create image view %u: %d", i, vr);
            return NK_ERROR_VULKAN;
        }
    }
    return NK_SUCCESS;
}

NkResult nk_swapchain_create(NkVkContext *vk, VkSurfaceKHR surface,
                              uint32_t width, uint32_t height,
                              NkSwapchain *out) {
    memset(out, 0, sizeof(*out));
    out->surface = surface;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->physical_device, surface, &caps);

    VkSurfaceFormatKHR fmt = choose_surface_format(vk->physical_device, surface);
    VkPresentModeKHR pm = choose_present_mode(vk->physical_device, surface);
    out->present_mode = pm;

    out->format = fmt.format;
    out->extent = (VkExtent2D){ width, height };

    // Clamp to capabilities
    if (out->extent.width  < caps.minImageExtent.width)  out->extent.width  = caps.minImageExtent.width;
    if (out->extent.width  > caps.maxImageExtent.width)  out->extent.width  = caps.maxImageExtent.width;
    if (out->extent.height < caps.minImageExtent.height) out->extent.height = caps.minImageExtent.height;
    if (out->extent.height > caps.maxImageExtent.height) out->extent.height = caps.maxImageExtent.height;

    uint32_t img_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && img_count > caps.maxImageCount)
        img_count = caps.maxImageCount;

    VkSwapchainCreateInfoKHR ci = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = surface,
        .minImageCount    = img_count,
        .imageFormat      = fmt.format,
        .imageColorSpace  = fmt.colorSpace,
        .imageExtent      = out->extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = caps.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = pm,
        .clipped          = VK_TRUE,
    };

    VkResult vr = vkCreateSwapchainKHR(vk->device, &ci, NULL, &out->swapchain);
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkCreateSwapchainKHR failed: %d", vr);
        return NK_ERROR_VULKAN;
    }

    // Get images
    vkGetSwapchainImagesKHR(vk->device, out->swapchain, &out->image_count, NULL);
    out->images = calloc(out->image_count, sizeof(VkImage));
    vkGetSwapchainImagesKHR(vk->device, out->swapchain, &out->image_count, out->images);

    NK_LOG_INFO("Swapchain created: %ux%u, %u images, format %d",
                out->extent.width, out->extent.height, out->image_count, out->format);

    return create_image_views(vk, out);
}

NkResult nk_swapchain_recreate(NkVkContext *vk, NkSwapchain *sc,
                                uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(vk->device);
    VkSurfaceKHR surface = sc->surface;

    // Destroy swapchain internals but keep the surface alive
    for (uint32_t i = 0; i < sc->image_count; i++) {
        if (sc->views && sc->views[i]) {
            vkDestroyImageView(vk->device, sc->views[i], NULL);
        }
    }
    free(sc->views);
    free(sc->images);
    if (sc->swapchain) {
        vkDestroySwapchainKHR(vk->device, sc->swapchain, NULL);
    }
    sc->views     = NULL;
    sc->images    = NULL;
    sc->swapchain = VK_NULL_HANDLE;
    sc->surface   = VK_NULL_HANDLE;

    NkResult r = nk_swapchain_create(vk, surface, width, height, sc);
    return r;
}

void nk_swapchain_destroy(NkVkContext *vk, NkSwapchain *sc) {
    if (!sc) return;
    for (uint32_t i = 0; i < sc->image_count; i++) {
        if (sc->views && sc->views[i]) {
            vkDestroyImageView(vk->device, sc->views[i], NULL);
        }
    }
    free(sc->views);
    free(sc->images);
    if (sc->swapchain) {
        vkDestroySwapchainKHR(vk->device, sc->swapchain, NULL);
    }
    if (sc->surface) {
        vkDestroySurfaceKHR(vk->instance, sc->surface, NULL);
    }
    sc->views      = NULL;
    sc->images     = NULL;
    sc->swapchain  = VK_NULL_HANDLE;
    sc->surface    = VK_NULL_HANDLE;
}
