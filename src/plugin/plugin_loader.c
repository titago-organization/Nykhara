/**
 * @file plugin_loader.c
 * @brief Plugin loader implementation — dlopen/dlsym/dlclose.
 */

#include "plugin_loader.h"
#include "../core/nk_log.h"

#include <dlfcn.h>
#include <string.h>

NkResult nk_plugin_load(const char *path, NkLoadedPlugin *out) {
    memset(out, 0, sizeof(*out));
    strncpy(out->path, path, sizeof(out->path) - 1);

    out->handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!out->handle) {
        NK_LOG_ERROR("dlopen(%s): %s", path, dlerror());
        return NK_ERROR_PLUGIN;
    }

    // POSIX guarantees dlsym returns a function pointer, but ISO C forbids
    // the cast from void* to function pointer. Use memcpy to satisfy pedantic mode.
    NkPluginDescribeFn describe;
    void *sym = dlsym(out->handle, "nk_plugin_describe");
    memcpy(&describe, &sym, sizeof(sym));
    if (!describe) {
        NK_LOG_ERROR("Plugin %s missing nk_plugin_describe symbol", path);
        dlclose(out->handle);
        return NK_ERROR_PLUGIN;
    }

    out->descriptor = describe();
    if (!out->descriptor) {
        NK_LOG_ERROR("Plugin %s: nk_plugin_describe returned NULL", path);
        dlclose(out->handle);
        return NK_ERROR_PLUGIN;
    }

    if (out->descriptor->abi_version != NK_PLUGIN_ABI_VERSION) {
        NK_LOG_ERROR("Plugin %s: ABI version mismatch (got %u, expected %u)",
                     path, out->descriptor->abi_version, NK_PLUGIN_ABI_VERSION);
        dlclose(out->handle);
        return NK_ERROR_PLUGIN;
    }

    out->loaded = true;
    NK_LOG_INFO("Plugin loaded: %s v%s", out->descriptor->name, out->descriptor->version);
    return NK_SUCCESS;
}

void nk_plugin_unload(NkLoadedPlugin *plugin) {
    if (!plugin || !plugin->loaded) return;

    NK_LOG_INFO("Unloading plugin: %s", plugin->descriptor->name);

    if (plugin->handle) {
        dlclose(plugin->handle);
    }
    memset(plugin, 0, sizeof(*plugin));
}

