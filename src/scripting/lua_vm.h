/**
 * @file lua_vm.h
 * @brief LuaJIT VM lifecycle — init, exec, hot-reload.
 */

#ifndef NK_LUA_VM_H
#define NK_LUA_VM_H

#include <nykhara/types.h>

typedef struct NkWidget NkWidget;

typedef struct lua_State lua_State;

typedef struct NkLuaVM {
    lua_State   *L;
    char         current_script[256];
    bool         initialized;
    int          root_ref;
} NkLuaVM;

/**
 * Initialize the Lua VM with sandboxed standard libraries.
 */
NkResult nk_lua_vm_init(NkLuaVM *vm);

/**
 * Execute a Lua script file.
 */
NkResult nk_lua_vm_exec(NkLuaVM *vm, const char *path);

/**
 * Hot-reload the currently loaded script.
 */
NkResult nk_lua_vm_reload(NkLuaVM *vm);

/**
 * Build a widget tree from the top-level Lua return value.
 */
NkResult nk_lua_vm_build_ui(NkLuaVM *vm, NkWidget **out_root);

/**
 * Invoke Lua on_click callback associated with a widget.
 */
NkResult nk_lua_vm_invoke_on_click(NkLuaVM *vm, const NkWidget *widget);

/**
 * Release Lua registry refs owned by a widget subtree.
 */
void nk_lua_vm_release_widget_refs(NkLuaVM *vm, NkWidget *root);

/**
 * Shutdown the Lua VM.
 */
void nk_lua_vm_shutdown(NkLuaVM *vm);

#endif // NK_LUA_VM_H

