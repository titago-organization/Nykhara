/**
 * @file nykhara.h
 * @brief Umbrella header for the Nykhara GUI framework.
 */

#ifndef NYKHARA_H
#define NYKHARA_H

#define NK_VERSION_MAJOR 0
#define NK_VERSION_MINOR 1
#define NK_VERSION_PATCH 0
#define NK_VERSION_STRING "0.1.0"

#include "types.h"
#include "plugin.h"
#include "widget.h"
#include "scripting.h"

/**
 * Application configuration passed to nk_app_create().
 */
typedef struct NkAppConfig {
    const char *app_name;
    const char *lua_entry;       // Path to the Lua entry-point script (e.g. "lua/init.lua")
    uint32_t    width;
    uint32_t    height;
    bool        debug_mode;      // Enable validation layers + Vulkan timestamps
} NkAppConfig;

/**
 * Create the application context (Wayland + Vulkan + Lua).
 */
NkResult nk_app_create(const NkAppConfig *config, NkApp **out_app);

/**
 * Run the main event loop until window close.
 */
NkResult nk_app_run(NkApp *app);

/**
 * Destroy the application and release all resources.
 */
void nk_app_destroy(NkApp *app);

#endif // NYKHARA_H

