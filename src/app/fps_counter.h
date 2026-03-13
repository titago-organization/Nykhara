/**
 * @file fps_counter.h
 * @brief FPS measurement and widget-label updater.
 */

#ifndef NK_FPS_COUNTER_H
#define NK_FPS_COUNTER_H

#include "../widget/widget_base.h"

typedef struct NkFpsCounter {
    uint32_t frame_count;
    double   last_time;
    float    current;
} NkFpsCounter;

/**
 * Initialise the counter (call once after getting the first timestamp).
 */
void nk_fps_init(NkFpsCounter *fps, double now);

/**
 * Call once per frame.  Returns true when the displayed value was updated
 * (roughly every 0.5 s), so you can refresh the window title.
 */
bool nk_fps_tick(NkFpsCounter *fps, double now);

/**
 * Write the current FPS into a label widget's text.
 */
void nk_fps_update_label(const NkFpsCounter *fps, NkWidget *label);

/**
 * Recursively search the tree for the first label whose text starts with "FPS".
 */
NkWidget *nk_fps_find_label(NkWidget *root);

#endif /* NK_FPS_COUNTER_H */
