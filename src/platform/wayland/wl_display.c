/**
 * @file wl_display.c
 * @brief Wayland display, xdg-shell window creation, and event dispatch.
 */

#include "wl_display.h"
#include "../../core/nk_log.h"

// Generated xdg-shell protocol header (built by meson custom_target)
#include "xdg-shell-client-protocol.h"

#include <string.h>
#include <stdlib.h>
#include <poll.h>

// ──────────────────────────── wl_output listener ────────────────────────────

static void output_geometry(void *data, struct wl_output *output,
                            int32_t x, int32_t y,
                            int32_t physical_width, int32_t physical_height,
                            int32_t subpixel,
                            const char *make, const char *model,
                            int32_t transform) {
    (void)data; (void)output; (void)x; (void)y;
    (void)physical_width; (void)physical_height;
    (void)subpixel; (void)make; (void)model; (void)transform;
}

static void output_mode(void *data, struct wl_output *output,
                        uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
    (void)output; (void)refresh;
    NkWlDisplay *wl = data;
    if ((flags & WL_OUTPUT_MODE_CURRENT) || (flags & WL_OUTPUT_MODE_PREFERRED)) {
        if (width > 0 && height > 0) {
            wl->output_width = (uint32_t)width;
            wl->output_height = (uint32_t)height;
        }
    }
}

static void output_done(void *data, struct wl_output *output) {
    (void)data; (void)output;
}

static void output_scale(void *data, struct wl_output *output, int32_t factor) {
    (void)output;
    NkWlDisplay *wl = data;
    if (factor >= 1) {
        wl->output_scale = factor;
        NK_LOG_DEBUG("Output scale: %d", factor);
    }
}

static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
};

// ──────────────────────────── wl_pointer listener ────────────────────────────

static void pointer_enter(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface,
                          wl_fixed_t sx, wl_fixed_t sy) {
    (void)pointer; (void)serial; (void)surface;
    NkWlDisplay *wl = data;
    wl->pointer_inside = true;
    wl->pointer_x = wl_fixed_to_double(sx);
    wl->pointer_y = wl_fixed_to_double(sy);
}

static void pointer_leave(void *data, struct wl_pointer *pointer,
                          uint32_t serial, struct wl_surface *surface) {
    (void)pointer; (void)serial; (void)surface;
    NkWlDisplay *wl = data;
    wl->pointer_inside = false;
}

static void pointer_motion(void *data, struct wl_pointer *pointer,
                           uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
    (void)pointer; (void)time;
    NkWlDisplay *wl = data;
    wl->pointer_x = wl_fixed_to_double(sx);
    wl->pointer_y = wl_fixed_to_double(sy);
}

static void pointer_button(void *data, struct wl_pointer *pointer,
                           uint32_t serial, uint32_t time,
                           uint32_t button, uint32_t state) {
    (void)pointer; (void)serial; (void)time;
    NkWlDisplay *wl = data;
    wl->pointer_button = button;
    if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
        wl->pointer_pressed_edge = true;
    } else {
        wl->pointer_released_edge = true;
    }
}

static void pointer_axis(void *data, struct wl_pointer *pointer,
                         uint32_t time, uint32_t axis, wl_fixed_t value) {
    (void)data; (void)pointer; (void)time; (void)axis; (void)value;
}

static void pointer_frame(void *data, struct wl_pointer *pointer) {
    (void)data; (void)pointer;
}

static void pointer_axis_source(void *data, struct wl_pointer *pointer, uint32_t axis_source) {
    (void)data; (void)pointer; (void)axis_source;
}

static void pointer_axis_stop(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis) {
    (void)data; (void)pointer; (void)time; (void)axis;
}

static void pointer_axis_discrete(void *data, struct wl_pointer *pointer, uint32_t axis, int32_t discrete) {
    (void)data; (void)pointer; (void)axis; (void)discrete;
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
    .frame = pointer_frame,
    .axis_source = pointer_axis_source,
    .axis_stop = pointer_axis_stop,
    .axis_discrete = pointer_axis_discrete,
};

// ──────────────────────────── wl_seat listener ────────────────────────────

static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t caps) {
    NkWlDisplay *wl = data;

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !wl->pointer) {
        wl->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(wl->pointer, &pointer_listener, wl);
        NK_LOG_DEBUG("Wayland pointer ready");
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wl->pointer) {
        wl_pointer_destroy(wl->pointer);
        wl->pointer = NULL;
    }
}

static void seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)data; (void)seat; (void)name;
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};

// ──────────────────────────── xdg_wm_base listener ────────────────────────────

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

// ──────────────────────────── Registry listener ────────────────────────────

static void registry_global(void *data, struct wl_registry *reg,
                             uint32_t name, const char *interface, uint32_t version) {
    NkWlDisplay *wl = data;
    (void)version;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        wl->compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
        NK_LOG_DEBUG("Bound wl_compositor v4");
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        wl->shm = wl_registry_bind(reg, name, &wl_shm_interface, 1);
        NK_LOG_DEBUG("Bound wl_shm");
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        wl->seat = wl_registry_bind(reg, name, &wl_seat_interface, 5);
        wl_seat_add_listener(wl->seat, &seat_listener, wl);
        NK_LOG_DEBUG("Bound wl_seat v5");
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        if (!wl->output) {
            wl->output = wl_registry_bind(reg, name, &wl_output_interface, 2);
            wl_output_add_listener(wl->output, &output_listener, wl);
            NK_LOG_DEBUG("Bound wl_output");
        }
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        wl->xdg_wm_base = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(wl->xdg_wm_base, &wm_base_listener, wl);
        NK_LOG_DEBUG("Bound xdg_wm_base");
    }
}

static void registry_global_remove(void *data, struct wl_registry *reg, uint32_t name) {
    (void)data; (void)reg; (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global        = registry_global,
    .global_remove = registry_global_remove,
};

// ──────────────────────────── Display API ────────────────────────────

NkResult nk_wl_display_connect(NkWlDisplay *wl) {
    memset(wl, 0, sizeof(*wl));

    wl->display = wl_display_connect(NULL);
    if (!wl->display) {
        NK_LOG_ERROR("Failed to connect to Wayland display");
        return NK_ERROR_WAYLAND;
    }

    wl->registry = wl_display_get_registry(wl->display);
    wl_registry_add_listener(wl->registry, &registry_listener, wl);

    // Two roundtrips: first to receive globals, second to receive events from binding
    wl_display_roundtrip(wl->display);
    wl_display_roundtrip(wl->display);

    if (!wl->compositor) {
        NK_LOG_ERROR("wl_compositor not found");
        return NK_ERROR_WAYLAND;
    }
    if (!wl->xdg_wm_base) {
        NK_LOG_ERROR("xdg_wm_base not found — compositor doesn't support xdg-shell");
        return NK_ERROR_WAYLAND;
    }

    wl->fd = wl_display_get_fd(wl->display);
    wl->running = true;

    NK_LOG_INFO("Connected to Wayland display");
    return NK_SUCCESS;
}

NkResult nk_wl_display_dispatch(NkWlDisplay *wl) {
    // Drain any events already decoded in the client-side queue
    // before attempting to prepare_read (required by Wayland API).
    while (wl_display_prepare_read(wl->display) != 0) {
        wl_display_dispatch_pending(wl->display);
    }

    // Flush outgoing requests
    if (wl_display_flush(wl->display) < 0) {
        wl_display_cancel_read(wl->display);
        NK_LOG_ERROR("Wayland flush error");
        wl->running = false;
        return NK_ERROR_WAYLAND;
    }

    // Poll the Wayland fd for incoming data (non-blocking, timeout=0)
    struct pollfd pfd = { .fd = wl->fd, .events = POLLIN };
    int ret = poll(&pfd, 1, 0);

    if (ret > 0) {
        // Data available — read events from the socket
        wl_display_read_events(wl->display);
    } else {
        // No data or error — cancel the read lock
        wl_display_cancel_read(wl->display);
    }

    // Dispatch any newly-decoded events
    wl_display_dispatch_pending(wl->display);

    return NK_SUCCESS;
}

void nk_wl_display_disconnect(NkWlDisplay *wl) {
    if (wl->pointer)      wl_pointer_destroy(wl->pointer);
    if (wl->output)       wl_output_destroy(wl->output);
    if (wl->xdg_wm_base) xdg_wm_base_destroy(wl->xdg_wm_base);
    if (wl->seat)         wl_seat_destroy(wl->seat);
    if (wl->shm)          wl_shm_destroy(wl->shm);
    if (wl->compositor)   wl_compositor_destroy(wl->compositor);
    if (wl->registry)     wl_registry_destroy(wl->registry);
    if (wl->display)      wl_display_disconnect(wl->display);
    memset(wl, 0, sizeof(*wl));
    NK_LOG_INFO("Disconnected from Wayland display");
}

void nk_wl_display_consume_pointer(NkWlDisplay *wl,
                                   double *x, double *y,
                                   bool *inside,
                                   bool *pressed_edge,
                                   bool *released_edge,
                                   uint32_t *button) {
    if (x) *x = wl->pointer_x;
    if (y) *y = wl->pointer_y;
    if (inside) *inside = wl->pointer_inside;
    if (pressed_edge) *pressed_edge = wl->pointer_pressed_edge;
    if (released_edge) *released_edge = wl->pointer_released_edge;
    if (button) *button = wl->pointer_button;

    wl->pointer_pressed_edge = false;
    wl->pointer_released_edge = false;
}

// ──────────────────────────── xdg_surface listener ────────────────────────────

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    NkWlWindow *win = data;
    xdg_surface_ack_configure(xdg_surface, serial);
    win->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

// ──────────────────────────── xdg_toplevel listener ────────────────────────────

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                                    int32_t width, int32_t height, struct wl_array *states) {
    NkWlWindow *win = data;
    (void)toplevel;

    win->fullscreen = false;
    uint32_t *state = NULL;
    wl_array_for_each(state, states) {
        if (*state == XDG_TOPLEVEL_STATE_FULLSCREEN) {
            win->fullscreen = true;
            break;
        }
    }

    // width/height of 0 means "use whatever you want"
    if (width > 0 && height > 0) {
        if ((uint32_t)width != win->width || (uint32_t)height != win->height) {
            win->width          = (uint32_t)width;
            win->height         = (uint32_t)height;
            win->resize_pending = true;
        }
    }
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel) {
    NkWlWindow *win = data;
    (void)toplevel;
    win->closed = true;
    NK_LOG_INFO("Window close requested");
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close     = xdg_toplevel_close,
};

// ──────────────────────────── Window API ────────────────────────────

NkResult nk_wl_window_create(NkWlDisplay *wl, NkWlWindow *win,
                              const char *title, uint32_t width, uint32_t height) {
    memset(win, 0, sizeof(*win));

    int32_t scale = wl->output_scale >= 1 ? wl->output_scale : 1;
    win->scale = scale;

    if (width == 0 || height == 0) {
        if (wl->output_width > 0 && wl->output_height > 0) {
            /* output_mode gives physical pixels — divide by scale for logical */
            win->width  = wl->output_width / (uint32_t)scale;
            win->height = wl->output_height / (uint32_t)scale;
        } else {
            win->width  = 1280;
            win->height = 720;
        }
    } else {
        win->width  = width;
        win->height = height;
    }

    // 1. Create wl_surface
    win->surface = wl_compositor_create_surface(wl->compositor);
    if (!win->surface) {
        NK_LOG_ERROR("Failed to create wl_surface");
        return NK_ERROR_WAYLAND;
    }

    // 2. Create xdg_surface
    win->xdg_surface = xdg_wm_base_get_xdg_surface(wl->xdg_wm_base, win->surface);
    if (!win->xdg_surface) {
        NK_LOG_ERROR("Failed to create xdg_surface");
        return NK_ERROR_WAYLAND;
    }
    xdg_surface_add_listener(win->xdg_surface, &xdg_surface_listener, win);

    // 3. Create xdg_toplevel (this makes it a real window)
    win->xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surface);
    if (!win->xdg_toplevel) {
        NK_LOG_ERROR("Failed to create xdg_toplevel");
        return NK_ERROR_WAYLAND;
    }
    xdg_toplevel_add_listener(win->xdg_toplevel, &xdg_toplevel_listener, win);
    xdg_toplevel_set_title(win->xdg_toplevel, title);
    xdg_toplevel_set_app_id(win->xdg_toplevel, "nykhara");

    // 4. Set buffer scale for HiDPI
    wl_surface_set_buffer_scale(win->surface, scale);

    // 5. Commit the surface to trigger the initial configure
    wl_surface_commit(win->surface);

    // 5. Wait for the first configure event
    wl_display_roundtrip(wl->display);

    if (!win->configured) {
        // Do another roundtrip just in case
        wl_display_roundtrip(wl->display);
    }

    NK_LOG_INFO("Window created: \"%s\" %ux%u", title, win->width, win->height);
    return NK_SUCCESS;
}

void nk_wl_window_destroy(NkWlDisplay *wl, NkWlWindow *win) {
    (void)wl;
    if (win->xdg_toplevel) xdg_toplevel_destroy(win->xdg_toplevel);
    if (win->xdg_surface)  xdg_surface_destroy(win->xdg_surface);
    if (win->surface)       wl_surface_destroy(win->surface);
    memset(win, 0, sizeof(*win));
    NK_LOG_INFO("Window destroyed");
}
