/**
 * @file scripting.h
 * @brief Public API for Lua-facing widget/animation registration.
 */

#ifndef NK_SCRIPTING_H
#define NK_SCRIPTING_H

#include "types.h"
#include "widget.h"

// Forward
typedef struct lua_State lua_State;

/**
 * Initialize the Lua VM, load sandboxed standard libs.
 */
NkResult nk_lua_init(void);

/**
 * Shut down the Lua VM and free all resources.
 */
void nk_lua_shutdown(void);

/**
 * Execute a Lua UI script file (builds the widget tree).
 */
NkResult nk_lua_exec_file(const char *path);

/**
 * Hot-reload: re-execute the currently loaded script.
 */
NkResult nk_lua_hot_reload(void);

/**
 * Register a C widget constructor so Lua can call `Widget.Button{}` etc.
 */
NkResult nk_lua_register_widget(const char *name, const NkWidgetVTable *vt, size_t struct_size);

/**
 * Get the raw lua_State* (for plugins that need direct access).
 */
lua_State *nk_lua_state(void);

#endif // NK_SCRIPTING_H

