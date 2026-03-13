/**
 * @file main.c
 * @brief Nykhara application entry point — bootstraps all subsystems,
 *        runs the main loop, and tears everything down.
 */

#include <nykhara/nykhara.h>

#include "../core/nk_log.h"
#include "../platform/wayland/wl_display.h"
#include "../render/vk_init.h"
#include "../render/vk_swapchain.h"
#include "../render/vk_cmd.h"
#include "../render/vk_render.h"
#include "../render/sdf_renderer.h"
#include "../input/input_mgr.h"
#include "../scripting/lua_vm.h"
#include "../animation/anim_engine.h"
#include "../layout/flex.h"
#include "../widget/widget_base.h"

#include "morph_demo.h"
#include "fallback_ui.h"
#include "fps_counter.h"

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

/* ═══════════════════════════════════════════════════════════════════
 *  App context
 * ═══════════════════════════════════════════════════════════════════ */

struct NkApp {
    NkAppConfig      config;

    /* Platform */
    NkWlDisplay      wayland;
    NkWlWindow       window;

    /* Vulkan */
    NkVkContext      vk;
    NkSwapchain      swapchain;
    NkFrameSync      frames[NK_MAX_FRAMES_IN_FLIGHT];
    NkSwapchainSync  sc_sync;
    NkSdfRenderer    sdf;

    /* Subsystems */
    NkInputMgr       input;
    NkLuaVM          lua;
    NkAnimEngine     anim;

    /* UI */
    NkWidget        *root_widget;
    NkWidget        *pressed_widget;
    NkWidget        *fps_label_widget;

    /* Timing */
    double           last_time;
    double           start_time;
    uint32_t         frame_index;
    bool             running;

    /* FPS */
    NkFpsCounter     fps;

    /* Lua hot-reload */
    time_t           lua_last_mtime;
    double           lua_watch_accum;
};

/* ═══════════════════════════════════════════════════════════════════
 *  Helpers
 * ═══════════════════════════════════════════════════════════════════ */

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static bool file_mtime(const char *path, time_t *out) {
    if (!path || !out) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    *out = st.st_mtime;
    return true;
}

/* ── Pointer / hover management ───────────────────────────────────── */

static void clear_hover_recursive(NkWidget *w) {
    if (!w) return;
    ((NkBasicWidget *)w)->hovered = false;
    for (NkWidget *c = w->first_child; c; c = c->next_sibling) {
        clear_hover_recursive(c);
    }
}

static void process_pointer(NkApp *app) {
    if (!app->root_widget) return;

    double   px = 0.0, py = 0.0;
    bool     inside = false, pressed_edge = false, released_edge = false;
    uint32_t button = 0;
    nk_wl_display_consume_pointer(&app->wayland,
                                  &px, &py, &inside,
                                  &pressed_edge, &released_edge, &button);
    (void)button;

    /* Wayland pointer is in logical pixels; layout is in physical pixels */
    int32_t s = app->window.scale >= 1 ? app->window.scale : 1;
    float spx = (float)px * (float)s;
    float spy = (float)py * (float)s;

    NkWidget *hovered = NULL;
    if (inside) hovered = nk_widget_hit_test(app->root_widget, spx, spy);

    clear_hover_recursive(app->root_widget);
    if (hovered) {
        ((NkBasicWidget *)hovered)->hovered = true;
    }

    if (pressed_edge && hovered) {
        app->pressed_widget = hovered;
        ((NkBasicWidget *)hovered)->pressed = true;
    }

    if (released_edge) {
        NkWidget *pressed = app->pressed_widget;
        if (pressed) {
            ((NkBasicWidget *)pressed)->pressed = false;
            if (pressed == hovered && nk_widget_activate(pressed)) {
                (void)nk_lua_vm_invoke_on_click(&app->lua, pressed);
            }
        }
        app->pressed_widget = NULL;
    }
}

/* ── Lua hot-reload ───────────────────────────────────────────────── */

static void rebuild_ui_from_lua(NkApp *app) {
    NkWidget *new_root = NULL;
    if (nk_lua_vm_build_ui(&app->lua, &new_root) != NK_SUCCESS || !new_root) return;

    if (app->root_widget) {
        nk_lua_vm_release_widget_refs(&app->lua, app->root_widget);
        nk_widget_destroy(app->root_widget);
    }

    app->root_widget      = new_root;
    app->pressed_widget   = NULL;
    app->fps_label_widget = nk_fps_find_label(app->root_widget);
}

static void hot_reload_if_needed(NkApp *app, float dt) {
    if (!app->config.lua_entry) return;

    app->lua_watch_accum += dt;
    if (app->lua_watch_accum < 0.2) return;
    app->lua_watch_accum = 0.0;

    time_t mt;
    if (!file_mtime(app->config.lua_entry, &mt)) return;
    if (app->lua_last_mtime == 0) { app->lua_last_mtime = mt; return; }
    if (mt == app->lua_last_mtime) return;

    app->lua_last_mtime = mt;
    if (nk_lua_vm_reload(&app->lua) == NK_SUCCESS) {
        rebuild_ui_from_lua(app);
        NK_LOG_INFO("Lua UI hot-reloaded");
    } else {
        NK_LOG_WARN("Lua hot-reload failed; keeping previous UI");
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  App lifecycle
 * ═══════════════════════════════════════════════════════════════════ */

NkResult nk_app_create(const NkAppConfig *config, NkApp **out_app) {
    NkApp *app = calloc(1, sizeof(NkApp));
    if (!app) return NK_ERROR_OUT_OF_MEMORY;

    app->config = *config;
    uint32_t w = config->width;
    uint32_t h = config->height;

    nk_log_set_level(config->debug_mode ? NK_LOG_TRACE : NK_LOG_INFO);
    NK_LOG_INFO("Nykhara %s starting...", NK_VERSION_STRING);

    /* ── Wayland ── */
    NkResult r = nk_wl_display_connect(&app->wayland);
    if (r != NK_SUCCESS) { free(app); return r; }

    r = nk_wl_window_create(&app->wayland, &app->window, config->app_name, w, h);
    if (r != NK_SUCCESS) { nk_wl_display_disconnect(&app->wayland); free(app); return r; }

    /* ── Vulkan ── */
    r = nk_vk_init(&app->vk, config->app_name, config->debug_mode);
    if (r != NK_SUCCESS) goto fail;

    /* Vulkan needs physical pixels = logical × scale */
    int32_t scale = app->window.scale >= 1 ? app->window.scale : 1;
    uint32_t pw = app->window.width  * (uint32_t)scale;
    uint32_t ph = app->window.height * (uint32_t)scale;

    VkSurfaceKHR surface = nk_vk_create_wayland_surface(
        &app->vk, app->wayland.display, app->window.surface);
    r = nk_swapchain_create(&app->vk, surface, pw, ph, &app->swapchain);
    if (r != NK_SUCCESS) goto fail;

    for (int i = 0; i < NK_MAX_FRAMES_IN_FLIGHT; i++) {
        r = nk_frame_sync_create(&app->vk, &app->frames[i]);
        if (r != NK_SUCCESS) goto fail;
    }
    r = nk_swapchain_sync_create(&app->vk, app->swapchain.image_count, &app->sc_sync);
    if (r != NK_SUCCESS) goto fail;

    r = nk_sdf_init(&app->sdf, &app->vk, app->swapchain.format);
    if (r != NK_SUCCESS) goto fail;

    /* ── Subsystems ── */
    nk_input_init(&app->input);
    nk_anim_init(&app->anim);

    r = nk_lua_vm_init(&app->lua);
    if (r != NK_SUCCESS) goto fail;

    if (config->lua_entry) {
        r = nk_lua_vm_exec(&app->lua, config->lua_entry);
        if (r == NK_SUCCESS) {
            rebuild_ui_from_lua(app);
            (void)file_mtime(config->lua_entry, &app->lua_last_mtime);
        } else {
            NK_LOG_WARN("Lua entry script failed, using fallback UI");
        }
    }

    if (!app->root_widget) {
        app->root_widget = nk_build_fallback_ui();
        if (!app->root_widget) { r = NK_ERROR_OUT_OF_MEMORY; goto fail; }
        app->fps_label_widget = nk_fps_find_label(app->root_widget);
    }

    /* ── Timing ── */
    double now = get_time_sec();
    app->frame_index    = 0;
    app->running        = true;
    app->last_time      = now;
    app->start_time     = now;
    app->lua_watch_accum = 0.0;

    nk_fps_init(&app->fps, now);

    *out_app = app;
    NK_LOG_INFO("Nykhara initialised (%ux%u)", app->window.width, app->window.height);
    return NK_SUCCESS;

fail:
    nk_app_destroy(app);
    return r;
}

NkResult nk_app_run(NkApp *app) {
    NK_LOG_INFO("Entering main loop");

    while (app->running && app->wayland.running && !app->window.closed) {
        nk_wl_display_dispatch(&app->wayland);

        double now = get_time_sec();
        float  dt  = (float)(now - app->last_time);
        app->last_time = now;

        nk_anim_tick(&app->anim, dt);
        hot_reload_if_needed(app, dt);

        /* ── FPS counter ── */
        if (nk_fps_tick(&app->fps, now)) {
            nk_fps_update_label(&app->fps, app->fps_label_widget);

            char title[128];
            snprintf(title, sizeof(title), "Nykhara Demo — %.1f FPS | %ux%u",
                     app->fps.current, app->window.width, app->window.height);
            xdg_toplevel_set_title(app->window.xdg_toplevel, title);
        }

        /* ── Resize ── */
        if (app->window.resize_pending) {
            app->window.resize_pending = false;
            wl_display_dispatch_pending(app->wayland.display);
            app->window.resize_pending = false;

            int32_t s = app->window.scale >= 1 ? app->window.scale : 1;
            uint32_t rpw = app->window.width  * (uint32_t)s;
            uint32_t rph = app->window.height * (uint32_t)s;

            nk_vk_wait_idle(&app->vk);
            nk_swapchain_sync_destroy(&app->vk, &app->sc_sync);
            nk_swapchain_recreate(&app->vk, &app->swapchain, rpw, rph);
            nk_swapchain_sync_create(&app->vk, app->swapchain.image_count, &app->sc_sync);
            continue;
        }

        /* ── Frame ── */
        uint32_t fi = app->frame_index % NK_MAX_FRAMES_IN_FLIGHT;
        NkFrameSync *frame = &app->frames[fi];

        uint32_t image_idx;
        NkResult r = nk_frame_begin(&app->vk, &app->swapchain, frame, &app->sc_sync, &image_idx);
        if (r != NK_SUCCESS) {
            int32_t s2 = app->window.scale >= 1 ? app->window.scale : 1;
            uint32_t fpw = app->window.width  * (uint32_t)s2;
            uint32_t fph = app->window.height * (uint32_t)s2;

            nk_vk_wait_idle(&app->vk);
            nk_swapchain_sync_destroy(&app->vk, &app->sc_sync);
            nk_swapchain_recreate(&app->vk, &app->swapchain, fpw, fph);
            nk_swapchain_sync_create(&app->vk, app->swapchain.image_count, &app->sc_sync);
            continue;
        }

        float W = (float)app->swapchain.extent.width;
        float H = (float)app->swapchain.extent.height;

        if (app->root_widget) {
            nk_flex_layout(app->root_widget->node, W, H);
            process_pointer(app);
            nk_widget_tick_animations(app->root_widget, dt);
        }

        VkCommandBuffer cmd = frame->cmd_buf;
        nk_render_begin(&app->vk, &app->swapchain, cmd, image_idx, 0.07f, 0.07f, 0.09f);
        nk_sdf_begin(&app->sdf, app->swapchain.extent);

        /* ── Widget tree ── */
        if (app->root_widget) {
            NkRenderContext rc = {
                .sdf      = &app->sdf,
                .offset_x = 0.0f,
                .offset_y = 0.0f,
                .time_sec = now - app->start_time,
            };
            nk_widget_draw_tree(app->root_widget, &rc);
        }

        /* ── Morph overlay ── */
        float time  = (float)(now - app->start_time);
        float base  = (W < H ? W : H) * 0.22f;
        float morph_radius = base + base * 0.05f * sinf(time * 1.4f);
        NkVec2  morph_center = { W * 0.5f, H * 0.5f };
        NkMorphState ms = nk_morph_state(time);
        NkColor morph_fill = { ms.color_r, ms.color_g, ms.color_b, ms.color_a };

        nk_sdf_morph(&app->sdf, morph_center, morph_radius,
                     ms.morph_t, ms.corner_r,
                     morph_fill, 0.0f, 1.0f);
        nk_sdf_morph(&app->sdf, morph_center, morph_radius + 6.0f,
                     ms.morph_t, ms.corner_r,
                     nk_color_hex(0xFFFFFFFF), 2.0f, 0.28f);

        nk_sdf_flush(&app->sdf, cmd);
        nk_render_end(&app->vk, &app->swapchain, cmd, image_idx);
        nk_frame_end(&app->vk, &app->swapchain, frame, &app->sc_sync, image_idx);

        app->frame_index++;
    }

    nk_vk_wait_idle(&app->vk);
    NK_LOG_INFO("Main loop exited");
    return NK_SUCCESS;
}

void nk_app_destroy(NkApp *app) {
    if (!app) return;
    NK_LOG_INFO("Shutting down Nykhara...");

    nk_vk_wait_idle(&app->vk);

    if (app->root_widget) {
        nk_lua_vm_release_widget_refs(&app->lua, app->root_widget);
        nk_widget_destroy(app->root_widget);
        app->root_widget = NULL;
    }

    nk_lua_vm_shutdown(&app->lua);
    nk_anim_cancel_all(&app->anim);

    nk_sdf_destroy(&app->sdf, &app->vk);
    nk_swapchain_sync_destroy(&app->vk, &app->sc_sync);
    for (int i = 0; i < NK_MAX_FRAMES_IN_FLIGHT; i++) {
        nk_frame_sync_destroy(&app->vk, &app->frames[i]);
    }

    nk_swapchain_destroy(&app->vk, &app->swapchain);
    nk_vk_shutdown(&app->vk);
    nk_wl_window_destroy(&app->wayland, &app->window);
    nk_wl_display_disconnect(&app->wayland);

    free(app);
}

/* ═══════════════════════════════════════════════════════════════════
 *  Entry point
 * ═══════════════════════════════════════════════════════════════════ */

int main(int argc, char **argv) {
    bool debug = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            debug = true;
        }
    }

    NkAppConfig config = {
        .app_name   = "Nykhara Demo",
        .lua_entry  = "lua/init.lua",
        .width      = 0,
        .height     = 0,
        .debug_mode = debug,
    };

    NkApp *app = NULL;
    NkResult r = nk_app_create(&config, &app);
    if (r != NK_SUCCESS) {
        NK_LOG_FATAL("Failed to create app: %d", r);
        return 1;
    }

    r = nk_app_run(app);
    nk_app_destroy(app);
    return r == NK_SUCCESS ? 0 : 1;
}
