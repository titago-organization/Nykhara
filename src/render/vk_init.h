/**
 * @file vk_init.h
 * @brief Vulkan instance/device initialization via volk.
 */

#ifndef NK_VK_INIT_H
#define NK_VK_INIT_H

#include <nykhara/types.h>

// We use volk — no need to link libvulkan at compile time.
// VK_NO_PROTOTYPES is set via build system (-DVK_NO_PROTOTYPES)
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <volk.h>

typedef struct NkVkContext {
    VkInstance                instance;
    VkPhysicalDevice          physical_device;
    VkDevice                  device;
    VkQueue                   graphics_queue;
    VkQueue                   present_queue;
    uint32_t                  graphics_family;
    uint32_t                  present_family;
    VkPhysicalDeviceProperties device_props;
    VkPhysicalDeviceMemoryProperties mem_props;
    VkDebugUtilsMessengerEXT  debug_messenger; // VK_NULL_HANDLE in release
    bool                      validation_enabled;
} NkVkContext;

/**
 * Initialize volk, create VkInstance, pick physical device, create VkDevice.
 * If debug_mode is true, enables validation layers and debug messenger.
 */
NkResult nk_vk_init(NkVkContext *ctx, const char *app_name, bool debug_mode);

/**
 * Destroy Vulkan device, instance and volk.
 */
void nk_vk_shutdown(NkVkContext *ctx);

/**
 * Find a memory type index matching requirements.
 */
int32_t nk_vk_find_memory_type(const NkVkContext *ctx, uint32_t type_filter, VkMemoryPropertyFlags props);

/**
 * Wait for device idle.
 */
void nk_vk_wait_idle(NkVkContext *ctx);

/**
 * Create a VkSurfaceKHR from Wayland display + wl_surface.
 */
VkSurfaceKHR nk_vk_create_wayland_surface(NkVkContext *ctx,
                                            struct wl_display *wl_display,
                                            struct wl_surface *wl_surface);

#endif // NK_VK_INIT_H

