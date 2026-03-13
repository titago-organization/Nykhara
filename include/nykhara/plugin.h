/**
 * @file plugin.h
 * @brief Plugin ABI — every plugin .so exports a NkPluginDescriptor.
 */

#ifndef NK_PLUGIN_H
#define NK_PLUGIN_H

#include "types.h"

#define NK_PLUGIN_ABI_VERSION 1

// Forward declare the app context plugins receive
typedef struct NkAppContext NkAppContext;

/**
 * Plugin lifecycle hooks.
 * Returned by every plugin's `nk_plugin_describe()` symbol.
 */
typedef struct NkPluginDescriptor {
    uint32_t    abi_version;          // Must equal NK_PLUGIN_ABI_VERSION
    const char *name;                 // Human-readable plugin name
    const char *version;              // Semver string
    const char *const *dependencies;  // NULL-terminated list of required plugin names

    NkResult (*on_load)  (NkAppContext *ctx);   // Called after dlopen
    void     (*on_unload)(NkAppContext *ctx);   // Called before dlclose
    NkResult (*on_reload)(NkAppContext *ctx);   // Hot-reload (optional, can be NULL)
} NkPluginDescriptor;

/**
 * Every plugin .so must export this symbol:
 *   const NkPluginDescriptor *nk_plugin_describe(void);
 */
typedef const NkPluginDescriptor *(*NkPluginDescribeFn)(void);

#endif // NK_PLUGIN_H

