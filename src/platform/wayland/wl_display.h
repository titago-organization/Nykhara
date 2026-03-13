/**
 * @file wl_display.h
 * @brief Wayland display connection, xdg-shell, and event loop.
 */

#ifndef NK_WL_DISPLAY_H
#define NK_WL_DISPLAY_H

#include <nykhara/types.h>
#include <wayland-client.h>

// Forward declare xdg types (from generated protocol header)
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

typedef struct NkWlDisplay {
    struct wl_display    *display;
    struct wl_registry   *registry;
    struct wl_compositor *compositor;
    struct wl_shm        *shm;
    struct wl_seat       *seat;
    struct wl_pointer    *pointer;
    struct wl_output     *output;
    struct xdg_wm_base   *xdg_wm_base;

    bool                  running;
    int                   fd;

    uint32_t              output_width;
    uint32_t              output_height;
    int32_t               output_scale;

    double                pointer_x;
    double                pointer_y;
    bool                  pointer_inside;
    bool                  pointer_pressed_edge;
    bool                  pointer_released_edge;
    uint32_t              pointer_button;
} NkWlDisplay;

typedef struct NkWlWindow {
    struct wl_surface    *surface;
    struct xdg_surface   *xdg_surface;
    struct xdg_toplevel  *xdg_toplevel;

    uint32_t              width;
    uint32_t              height;
    int32_t               scale;
    bool                  configured;
    bool                  closed;
    bool                  resize_pending;
    bool                  fullscreen;
} NkWlWindow;

/**
 * Connect to the Wayland compositor.
 */
NkResult nk_wl_display_connect(NkWlDisplay *wl);

/**
 * Dispatch events (non-blocking, call in main loop).
 */
NkResult nk_wl_display_dispatch(NkWlDisplay *wl);

/**
 * Disconnect from the Wayland compositor.
 */
void nk_wl_display_disconnect(NkWlDisplay *wl);

/**
 * Get pointer state and consume press/release edge flags.
 */
void nk_wl_display_consume_pointer(NkWlDisplay *wl,
                                   double *x, double *y,
                                   bool *inside,
                                   bool *pressed_edge,
                                   bool *released_edge,
                                   uint32_t *button);

NkResult nk_wl_window_create(NkWlDisplay *wl, NkWlWindow *win,
                              const char *title, uint32_t width, uint32_t height);
void     nk_wl_window_destroy(NkWlDisplay *wl, NkWlWindow *win);

#endif // NK_WL_DISPLAY_H

