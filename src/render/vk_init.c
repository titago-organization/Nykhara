/**
 * @file vk_init.c
 * @brief Vulkan bootstrap via volk — instance, device, queues.
 */

#include "vk_init.h"
#include "../core/nk_log.h"

#include <string.h>
#include <stdlib.h>

// ── Debug callback ──

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
               VkDebugUtilsMessageTypeFlagsEXT type,
               const VkDebugUtilsMessengerCallbackDataEXT *data,
               void *user) {
    (void)type; (void)user;
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        NK_LOG_WARN("[Vulkan] %s", data->pMessage);
    } else {
        NK_LOG_DEBUG("[Vulkan] %s", data->pMessage);
    }
    return VK_FALSE;
}

// ── Queue family selection ──

static bool find_queue_families(VkPhysicalDevice pd, uint32_t *gfx, uint32_t *present) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &count, NULL);
    VkQueueFamilyProperties *props = calloc(count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &count, props);

    bool found_gfx = false;
    for (uint32_t i = 0; i < count; i++) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            *gfx = i;
            *present = i; // On Wayland, graphics queue typically supports present
            found_gfx = true;
            break;
        }
    }
    free(props);
    return found_gfx;
}

// ── Pick a discrete GPU, fallback to integrated ──

static VkPhysicalDevice pick_physical_device(VkInstance instance) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, NULL);
    if (count == 0) return VK_NULL_HANDLE;

    VkPhysicalDevice *devices = calloc(count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &count, devices);

    VkPhysicalDevice best = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        NK_LOG_INFO("GPU[%u]: %s", i, props.deviceName);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            best = devices[i];
            break;
        }
        if (best == VK_NULL_HANDLE) best = devices[i];
    }
    free(devices);
    return best;
}

// ── Public API ──

// ── Layer / extension availability checks ──

static bool check_layer_available(const char *layer_name) {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, NULL);
    if (count == 0) return false;

    VkLayerProperties *props = calloc(count, sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&count, props);

    bool found = false;
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(props[i].layerName, layer_name) == 0) {
            found = true;
            break;
        }
    }
    free(props);
    return found;
}

static bool check_instance_extension_available(const char *ext_name) {
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    if (count == 0) return false;

    VkExtensionProperties *props = calloc(count, sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &count, props);

    bool found = false;
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(props[i].extensionName, ext_name) == 0) {
            found = true;
            break;
        }
    }
    free(props);
    return found;
}

// ── Public API ──

NkResult nk_vk_init(NkVkContext *ctx, const char *app_name, bool debug_mode) {
    memset(ctx, 0, sizeof(*ctx));

    // Initialize volk (loads vkGetInstanceProcAddr)
    VkResult vr = volkInitialize();
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("volk failed to initialize: %d", vr);
        return NK_ERROR_VULKAN;
    }

    // ── Check what debug features are actually available ──

    bool has_validation = false;
    bool has_debug_utils = false;

    if (debug_mode) {
        has_validation = check_layer_available("VK_LAYER_KHRONOS_validation");
        has_debug_utils = check_instance_extension_available(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        if (!has_validation) {
            NK_LOG_WARN("Validation layers requested but not available — "
                        "install vulkan-validation-layers for debug diagnostics");
        }
        if (!has_debug_utils) {
            NK_LOG_WARN("VK_EXT_debug_utils not available — debug messenger disabled");
        }
    }

    ctx->validation_enabled = has_validation;

    // ── Instance ──

    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = app_name,
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName        = "Nykhara",
        .engineVersion      = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion         = VK_API_VERSION_1_3,
    };

    // Build extension list dynamically
    const char *extensions[8];
    uint32_t ext_count = 0;
    extensions[ext_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
    extensions[ext_count++] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    if (has_debug_utils) {
        extensions[ext_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    // Build layer list dynamically
    const char *layers[4];
    uint32_t layer_count = 0;
    if (has_validation) {
        layers[layer_count++] = "VK_LAYER_KHRONOS_validation";
    }

    VkInstanceCreateInfo ci = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &app_info,
        .enabledExtensionCount   = ext_count,
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount       = layer_count,
        .ppEnabledLayerNames     = layers,
    };

    vr = vkCreateInstance(&ci, NULL, &ctx->instance);
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkCreateInstance failed: %d", vr);
        return NK_ERROR_VULKAN;
    }

    // Load instance-level functions
    volkLoadInstance(ctx->instance);

    // ── Debug messenger (only if extension is actually loaded) ──

    if (has_debug_utils) {
        VkDebugUtilsMessengerCreateInfoEXT dci = {
            .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug_callback,
        };
        vkCreateDebugUtilsMessengerEXT(ctx->instance, &dci, NULL, &ctx->debug_messenger);
        NK_LOG_INFO("Vulkan debug messenger enabled");
    }

    // ── Physical device ──

    ctx->physical_device = pick_physical_device(ctx->instance);
    if (ctx->physical_device == VK_NULL_HANDLE) {
        NK_LOG_ERROR("No Vulkan-capable GPU found");
        return NK_ERROR_VULKAN;
    }

    vkGetPhysicalDeviceProperties(ctx->physical_device, &ctx->device_props);
    vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &ctx->mem_props);
    NK_LOG_INFO("Selected GPU: %s", ctx->device_props.deviceName);

    // ── Queue families ──

    if (!find_queue_families(ctx->physical_device, &ctx->graphics_family, &ctx->present_family)) {
        NK_LOG_ERROR("No graphics queue family found");
        return NK_ERROR_VULKAN;
    }

    // ── Logical device ──

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = ctx->graphics_family,
        .queueCount       = 1,
        .pQueuePriorities = &priority,
    };

    const char *dev_exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkPhysicalDeviceVulkan13Features features13 = {
        .sType                          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering               = VK_TRUE,
        .synchronization2               = VK_TRUE,
        .shaderDemoteToHelperInvocation = VK_TRUE,
    };

    VkDeviceCreateInfo dci = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &features13,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &qci,
        .enabledExtensionCount   = 1,
        .ppEnabledExtensionNames = dev_exts,
    };

    vr = vkCreateDevice(ctx->physical_device, &dci, NULL, &ctx->device);
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkCreateDevice failed: %d", vr);
        return NK_ERROR_VULKAN;
    }

    volkLoadDevice(ctx->device);

    vkGetDeviceQueue(ctx->device, ctx->graphics_family, 0, &ctx->graphics_queue);
    ctx->present_queue = ctx->graphics_queue;

    NK_LOG_INFO("Vulkan initialized successfully (API 1.3)");
    return NK_SUCCESS;
}

void nk_vk_shutdown(NkVkContext *ctx) {
    if (ctx->device) {
        vkDestroyDevice(ctx->device, NULL);
    }
    if (ctx->debug_messenger) {
        vkDestroyDebugUtilsMessengerEXT(ctx->instance, ctx->debug_messenger, NULL);
    }
    if (ctx->instance) {
        vkDestroyInstance(ctx->instance, NULL);
    }
    memset(ctx, 0, sizeof(*ctx));
}

int32_t nk_vk_find_memory_type(const NkVkContext *ctx, uint32_t type_filter, VkMemoryPropertyFlags props) {
    for (uint32_t i = 0; i < ctx->mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1u << i)) &&
            (ctx->mem_props.memoryTypes[i].propertyFlags & props) == props) {
            return (int32_t)i;
        }
    }
    return -1;
}

void nk_vk_wait_idle(NkVkContext *ctx) {
    if (ctx->device) vkDeviceWaitIdle(ctx->device);
}

VkSurfaceKHR nk_vk_create_wayland_surface(NkVkContext *ctx,
                                            struct wl_display *wl_display,
                                            struct wl_surface *wl_surface) {
    VkWaylandSurfaceCreateInfoKHR ci = {
        .sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = wl_display,
        .surface = wl_surface,
    };
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult vr = vkCreateWaylandSurfaceKHR(ctx->instance, &ci, NULL, &surface);
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkCreateWaylandSurfaceKHR failed: %d", vr);
    }
    return surface;
}

