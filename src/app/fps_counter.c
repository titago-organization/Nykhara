/**
 * @file fps_counter.c
 * @brief FPS measurement — samples frame count every 0.5 s.
 */

#include "fps_counter.h"

#include <stdio.h>
#include <string.h>

void nk_fps_init(NkFpsCounter *fps, double now) {
    fps->frame_count = 0;
    fps->last_time   = now;
    fps->current     = 0.0f;
}

bool nk_fps_tick(NkFpsCounter *fps, double now) {
    fps->frame_count++;

    double elapsed = now - fps->last_time;
    if (elapsed < 0.5) return false;

    fps->current     = (float)fps->frame_count / (float)elapsed;
    fps->frame_count = 0;
    fps->last_time   = now;
    return true;
}

void nk_fps_update_label(const NkFpsCounter *fps, NkWidget *label) {
    if (!label) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "FPS: %.1f", fps->current);
    nk_widget_set_text(label, buf);
}

NkWidget *nk_fps_find_label(NkWidget *w) {
    if (!w) return NULL;

    NkBasicWidget *bw = (NkBasicWidget *)w;
    if (bw->kind == NK_WIDGET_LABEL) {
        const char *t = nk_widget_text(w);
        if (t && strncmp(t, "FPS", 3) == 0) return w;
    }

    for (NkWidget *c = w->first_child; c; c = c->next_sibling) {
        NkWidget *res = nk_fps_find_label(c);
        if (res) return res;
    }
    return NULL;
}
