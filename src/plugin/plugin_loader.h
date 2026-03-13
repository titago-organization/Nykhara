/**
 * @file plugin_loader.h
 * @brief Dynamic plugin loader — dlopen/dlsym wrapper.
 */

#ifndef NK_PLUGIN_LOADER_H
#define NK_PLUGIN_LOADER_H

#include <nykhara/plugin.h>

typedef struct NkLoadedPlugin {
    void                    *handle;     // dlopen handle
    const NkPluginDescriptor *descriptor;
    char                     path[256];
    bool                     loaded;
} NkLoadedPlugin;

/**
 * Load a plugin .so file.
 */
NkResult nk_plugin_load(const char *path, NkLoadedPlugin *out);

/**
 * Unload a plugin.
 */
void nk_plugin_unload(NkLoadedPlugin *plugin);

#endif // NK_PLUGIN_LOADER_H

