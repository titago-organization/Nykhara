/**
 * @file lua_vm.c
 * @brief LuaJIT VM implementation — lifecycle, module bootstrap, UI tree parsing.
 */

#include "lua_vm.h"
#include "../core/nk_log.h"
#include "../widget/widget_base.h"
#include "../layout/node.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>

static void apply_direction_from_type(const char *type_name, NkStyle *s) {
    if (strcmp(type_name, "Row") == 0) {
        s->direction = NK_FLEX_ROW;
    } else if (strcmp(type_name, "Column") == 0) {
        s->direction = NK_FLEX_COLUMN;
    }
}

static bool lua_get_number_field(lua_State *L, int idx, const char *name, float *out) {
    lua_getfield(L, idx, name);
    if (lua_isnumber(L, -1)) {
        *out = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

static bool lua_get_bool_field(lua_State *L, int idx, const char *name, bool *out) {
    lua_getfield(L, idx, name);
    if (lua_isboolean(L, -1)) {
        *out = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

static bool lua_get_string_field(lua_State *L, int idx, const char *name, const char **out) {
    lua_getfield(L, idx, name);
    if (lua_isstring(L, -1)) {
        *out = lua_tostring(L, -1);
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

static bool lua_parse_color(lua_State *L, int idx, const char *name, NkColor *out) {
    lua_getfield(L, idx, name);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    lua_rawgeti(L, -1, 1); float r = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L, 1);
    lua_rawgeti(L, -1, 2); float g = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L, 1);
    lua_rawgeti(L, -1, 3); float b = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L, 1);
    lua_rawgeti(L, -1, 4); float a = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 1.0f; lua_pop(L, 1);

    *out = (NkColor){ r, g, b, a };
    lua_pop(L, 1);
    return true;
}

static void lua_parse_edge_insets(lua_State *L, int idx, const char *name, NkEdgeInsets *out) {
    lua_getfield(L, idx, name);
    if (lua_isnumber(L, -1)) {
        float v = (float)lua_tonumber(L, -1);
        *out = (NkEdgeInsets){ v, v, v, v };
        lua_pop(L, 1);
        return;
    }

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    float top = out->top;
    float right = out->right;
    float bottom = out->bottom;
    float left = out->left;

    lua_getfield(L, -1, "all");
    if (lua_isnumber(L, -1)) {
        float v = (float)lua_tonumber(L, -1);
        top = right = bottom = left = v;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "top"); if (lua_isnumber(L, -1)) top = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, -1, "right"); if (lua_isnumber(L, -1)) right = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, -1, "bottom"); if (lua_isnumber(L, -1)) bottom = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, -1, "left"); if (lua_isnumber(L, -1)) left = (float)lua_tonumber(L, -1); lua_pop(L, 1);

    *out = (NkEdgeInsets){ top, right, bottom, left };
    lua_pop(L, 1);
}

static void lua_parse_corners(lua_State *L, int idx, NkCorners *out) {
    lua_getfield(L, idx, "corner_radius");
    if (lua_isnumber(L, -1)) {
        float v = (float)lua_tonumber(L, -1);
        *out = (NkCorners){ v, v, v, v };
        lua_pop(L, 1);
        return;
    }

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    float tl = out->top_left;
    float tr = out->top_right;
    float br = out->bottom_right;
    float bl = out->bottom_left;

    lua_getfield(L, -1, "all"); if (lua_isnumber(L, -1)) { float v = (float)lua_tonumber(L, -1); tl = tr = br = bl = v; } lua_pop(L, 1);
    lua_getfield(L, -1, "top_left"); if (lua_isnumber(L, -1)) tl = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, -1, "top_right"); if (lua_isnumber(L, -1)) tr = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, -1, "bottom_right"); if (lua_isnumber(L, -1)) br = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    lua_getfield(L, -1, "bottom_left"); if (lua_isnumber(L, -1)) bl = (float)lua_tonumber(L, -1); lua_pop(L, 1);

    *out = (NkCorners){ tl, tr, br, bl };
    lua_pop(L, 1);
}

static void apply_align_justify(lua_State *L, int idx, NkStyle *s) {
    const char *str = NULL;

    if (lua_get_string_field(L, idx, "align", &str) || lua_get_string_field(L, idx, "align_items", &str)) {
        if (strcmp(str, "start") == 0) s->align_items = NK_ALIGN_START;
        else if (strcmp(str, "center") == 0) s->align_items = NK_ALIGN_CENTER;
        else if (strcmp(str, "end") == 0) s->align_items = NK_ALIGN_END;
        else if (strcmp(str, "stretch") == 0) s->align_items = NK_ALIGN_STRETCH;
    }

    if (lua_get_string_field(L, idx, "justify", &str)) {
        if (strcmp(str, "start") == 0) s->justify = NK_JUSTIFY_START;
        else if (strcmp(str, "center") == 0) s->justify = NK_JUSTIFY_CENTER;
        else if (strcmp(str, "end") == 0) s->justify = NK_JUSTIFY_END;
        else if (strcmp(str, "space_between") == 0) s->justify = NK_JUSTIFY_SPACE_BETWEEN;
        else if (strcmp(str, "space_around") == 0) s->justify = NK_JUSTIFY_SPACE_AROUND;
        else if (strcmp(str, "space_evenly") == 0) s->justify = NK_JUSTIFY_SPACE_EVENLY;
    }
}

static void apply_props_to_widget(lua_State *L, int idx, const char *type_name, NkWidget *w) {
    NkStyle *s = &w->node->style;

    apply_direction_from_type(type_name, s);
    apply_align_justify(L, idx, s);

    lua_get_number_field(L, idx, "width", &s->width);
    lua_get_number_field(L, idx, "height", &s->height);
    lua_get_number_field(L, idx, "min_width", &s->min_width);
    lua_get_number_field(L, idx, "min_height", &s->min_height);
    lua_get_number_field(L, idx, "max_width", &s->max_width);
    lua_get_number_field(L, idx, "max_height", &s->max_height);
    lua_get_number_field(L, idx, "flex_grow", &s->flex_grow);
    lua_get_number_field(L, idx, "flex_shrink", &s->flex_shrink);
    lua_get_number_field(L, idx, "flex_basis", &s->flex_basis);
    lua_get_number_field(L, idx, "gap", &s->gap);
    lua_get_number_field(L, idx, "border_width", &s->border_width);

    lua_parse_edge_insets(L, idx, "padding", &s->padding);
    lua_parse_edge_insets(L, idx, "margin", &s->margin);
    lua_parse_corners(L, idx, &s->corner_radius);
    lua_parse_color(L, idx, "background", &s->background);
    lua_parse_color(L, idx, "border_color", &s->border_color);

    NkBasicWidget *bw = (NkBasicWidget *)w;
    if (bw->kind == NK_WIDGET_SWITCH) {
        lua_get_bool_field(L, idx, "value", &bw->toggled);
        lua_parse_color(L, idx, "track_on", &bw->color_on);
        lua_parse_color(L, idx, "track_off", &bw->color_off);
    }

    const char *text = NULL;
    if (lua_get_string_field(L, idx, "text", &text)) {
        nk_widget_set_text(w, text);
    }

    lua_get_number_field(L, idx, "font_size", &bw->font_size);

    if (bw->kind == NK_WIDGET_LABEL) {
        lua_parse_color(L, idx, "color", &bw->color_on);
    }

    lua_getfield(L, idx, "on_click");
    if (lua_isfunction(L, -1)) {
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        nk_widget_set_lua_callback_ref(w, ref);
    } else {
        lua_pop(L, 1);
    }

    nk_widget_apply_style_defaults(w);
}

static int l_widget_ctor(lua_State *L) {
    const char *type_name = lua_tostring(L, lua_upvalueindex(1));

    if (lua_isnoneornil(L, 1)) {
        lua_newtable(L);
    } else {
        luaL_checktype(L, 1, LUA_TTABLE);
        lua_pushvalue(L, 1);
    }

    lua_pushstring(L, type_name);
    lua_setfield(L, -2, "_type");
    return 1;
}

static void push_ctor(lua_State *L, const char *type_name) {
    lua_pushstring(L, type_name);
    lua_pushcclosure(L, l_widget_ctor, 1);
}

static void push_layouts_module(lua_State *L) {
    lua_newtable(L);
    push_ctor(L, "Row"); lua_setfield(L, -2, "Row");
    push_ctor(L, "Column"); lua_setfield(L, -2, "Column");
    push_ctor(L, "Box"); lua_setfield(L, -2, "Box");
}

static void push_nykhara_module(lua_State *L) {
    lua_newtable(L);

    push_ctor(L, "Box"); lua_setfield(L, -2, "Box");
    push_ctor(L, "Button"); lua_setfield(L, -2, "Button");
    push_ctor(L, "Switch"); lua_setfield(L, -2, "Switch");
    push_ctor(L, "Label"); lua_setfield(L, -2, "Label");
    push_ctor(L, "Row"); lua_setfield(L, -2, "Row");
    push_ctor(L, "Column"); lua_setfield(L, -2, "Column");

    push_layouts_module(L);
    lua_setfield(L, -2, "Layouts");
}

static void register_modules(lua_State *L) {
    // package.loaded["Nykhara"] and package.loaded["Nykhara.Layouts"]
    lua_getglobal(L, "package");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "loaded");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return;
    }

    push_nykhara_module(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "Nykhara");
    lua_pushvalue(L, -1);
    lua_setglobal(L, "Nk");
    lua_setfield(L, -2, "Nykhara");

    push_layouts_module(L);
    lua_setfield(L, -2, "Nykhara.Layouts");

    lua_pop(L, 2);
}

static NkResult build_widget_from_table(lua_State *L, int idx, NkWidget **out_widget);

static void append_children_from_table_array(lua_State *L, int idx, NkWidget *parent) {
    size_t n = (size_t)lua_objlen(L, idx);
    for (size_t i = 1; i <= n; i++) {
        lua_rawgeti(L, idx, (int)i);
        if (lua_istable(L, -1)) {
            NkWidget *child = NULL;
            if (build_widget_from_table(L, lua_gettop(L), &child) == NK_SUCCESS && child) {
                nk_widget_add_child(parent, child);
            }
        }
        lua_pop(L, 1);
    }
}

static void append_children_from_children_field(lua_State *L, int idx, NkWidget *parent) {
    lua_getfield(L, idx, "children");
    if (lua_istable(L, -1)) {
        int children_idx = lua_gettop(L);
        append_children_from_table_array(L, children_idx, parent);
    }
    lua_pop(L, 1);
}

static NkResult build_widget_from_table(lua_State *L, int idx, NkWidget **out_widget) {
    if (!lua_istable(L, idx)) return NK_ERROR_LUA;

    const char *type_name = NULL;
    lua_getfield(L, idx, "_type");
    if (lua_isstring(L, -1)) type_name = lua_tostring(L, -1);
    lua_pop(L, 1);

    if (!type_name) type_name = "Box";

    NkWidgetKind kind;
    if (!nk_widget_kind_from_type(type_name, &kind)) {
        NK_LOG_WARN("Unknown Lua widget type '%s', using Box", type_name);
        kind = NK_WIDGET_BOX;
    }

    NkWidget *w = nk_widget_create_builtin(kind);
    if (!w) return NK_ERROR_OUT_OF_MEMORY;

    apply_props_to_widget(L, idx, type_name, w);
    append_children_from_table_array(L, idx, w);
    append_children_from_children_field(L, idx, w);

    *out_widget = w;
    return NK_SUCCESS;
}

NkResult nk_lua_vm_init(NkLuaVM *vm) {
    memset(vm, 0, sizeof(*vm));

    vm->L = luaL_newstate();
    if (!vm->L) {
        NK_LOG_ERROR("Failed to create Lua state");
        return NK_ERROR_LUA;
    }

    static const luaL_Reg minimal_libs[] = {
        { "",              luaopen_base },
        { LUA_TABLIBNAME,  luaopen_table },
        { LUA_STRLIBNAME,  luaopen_string },
        { LUA_MATHLIBNAME, luaopen_math },
        { LUA_LOADLIBNAME, luaopen_package },
        { NULL, NULL },
    };
    for (const luaL_Reg *lib = minimal_libs; lib->func; lib++) {
        lua_pushcfunction(vm->L, lib->func);
        lua_pushstring(vm->L, lib->name);
        lua_call(vm->L, 1, 0);
    }

    lua_pushnil(vm->L); lua_setglobal(vm->L, "dofile");

    register_modules(vm->L);

    vm->initialized = true;
    vm->root_ref = LUA_NOREF;
    NK_LOG_INFO("LuaJIT VM initialized");
    return NK_SUCCESS;
}

NkResult nk_lua_vm_exec(NkLuaVM *vm, const char *path) {
    if (!vm->initialized || !vm->L) return NK_ERROR_LUA;

    strncpy(vm->current_script, path, sizeof(vm->current_script) - 1);
    vm->current_script[sizeof(vm->current_script) - 1] = '\0';

    lua_settop(vm->L, 0);

    int err = luaL_loadfile(vm->L, path);
    if (err != 0) {
        const char *msg = lua_tostring(vm->L, -1);
        NK_LOG_ERROR("Lua load error in %s: %s", path, msg ? msg : "(unknown)");
        lua_pop(vm->L, 1);
        return NK_ERROR_LUA;
    }

    err = lua_pcall(vm->L, 0, 1, 0);
    if (err != 0) {
        const char *msg = lua_tostring(vm->L, -1);
        NK_LOG_ERROR("Lua error in %s: %s", path, msg ? msg : "(unknown)");
        lua_pop(vm->L, 1);
        return NK_ERROR_LUA;
    }

    if (vm->root_ref != LUA_NOREF) {
        luaL_unref(vm->L, LUA_REGISTRYINDEX, vm->root_ref);
        vm->root_ref = LUA_NOREF;
    }

    if (!lua_istable(vm->L, -1)) {
        NK_LOG_WARN("Lua script %s returned non-table root, using empty UI", path);
        lua_pop(vm->L, 1);
    } else {
        vm->root_ref = luaL_ref(vm->L, LUA_REGISTRYINDEX);
    }

    NK_LOG_INFO("Lua script executed: %s", path);
    return NK_SUCCESS;
}

NkResult nk_lua_vm_reload(NkLuaVM *vm) {
    if (!vm->initialized || vm->current_script[0] == '\0') return NK_ERROR_LUA;
    NK_LOG_INFO("Hot-reloading: %s", vm->current_script);
    return nk_lua_vm_exec(vm, vm->current_script);
}

NkResult nk_lua_vm_build_ui(NkLuaVM *vm, NkWidget **out_root) {
    if (!vm || !vm->L || !out_root) return NK_ERROR_INVALID_ARG;
    *out_root = NULL;

    if (vm->root_ref == LUA_NOREF) {
        NK_LOG_WARN("No Lua UI root table was produced");
        return NK_ERROR_LUA;
    }

    lua_rawgeti(vm->L, LUA_REGISTRYINDEX, vm->root_ref);
    NkResult r = build_widget_from_table(vm->L, lua_gettop(vm->L), out_root);
    lua_pop(vm->L, 1);
    return r;
}

static void release_refs_recursive(lua_State *L, NkWidget *w) {
    if (!w) return;

    int ref = nk_widget_lua_callback_ref(w);
    if (ref >= 0) {
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
        nk_widget_set_lua_callback_ref(w, -2);
    }

    for (NkWidget *c = w->first_child; c; c = c->next_sibling) {
        release_refs_recursive(L, c);
    }
}

void nk_lua_vm_release_widget_refs(NkLuaVM *vm, NkWidget *root) {
    if (!vm || !vm->L || !root) return;
    release_refs_recursive(vm->L, root);
}

NkResult nk_lua_vm_invoke_on_click(NkLuaVM *vm, const NkWidget *widget) {
    if (!vm || !vm->L || !widget) return NK_ERROR_INVALID_ARG;

    int ref = nk_widget_lua_callback_ref(widget);
    if (ref < 0) return NK_ERROR_INVALID_ARG;

    lua_rawgeti(vm->L, LUA_REGISTRYINDEX, ref);
    if (!lua_isfunction(vm->L, -1)) {
        lua_pop(vm->L, 1);
        return NK_ERROR_LUA;
    }

    const NkBasicWidget *bw = (const NkBasicWidget *)widget;

    lua_newtable(vm->L);
    lua_pushinteger(vm->L, (lua_Integer)widget->id); lua_setfield(vm->L, -2, "id");
    lua_pushstring(vm->L, nk_widget_text(widget)); lua_setfield(vm->L, -2, "text");
    lua_pushboolean(vm->L, bw->toggled ? 1 : 0); lua_setfield(vm->L, -2, "value");

    if (lua_pcall(vm->L, 1, 0, 0) != 0) {
        const char *msg = lua_tostring(vm->L, -1);
        NK_LOG_ERROR("Lua on_click error: %s", msg ? msg : "(unknown)");
        lua_pop(vm->L, 1);
        return NK_ERROR_LUA;
    }

    return NK_SUCCESS;
}

void nk_lua_vm_shutdown(NkLuaVM *vm) {
    if (vm->L) {
        if (vm->root_ref != LUA_NOREF) {
            luaL_unref(vm->L, LUA_REGISTRYINDEX, vm->root_ref);
            vm->root_ref = LUA_NOREF;
        }
        lua_close(vm->L);
        vm->L = NULL;
    }
    vm->initialized = false;
    NK_LOG_INFO("LuaJIT VM shutdown");
}

