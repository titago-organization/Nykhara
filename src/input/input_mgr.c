/**
 * @file input_mgr.c
 * @brief Input manager — dispatches events to the registered handler.
 */

#include "input_mgr.h"
#include "../core/nk_log.h"

#include <string.h>

void nk_input_init(NkInputMgr *input) {
    memset(input, 0, sizeof(*input));
}

void nk_input_set_handler(NkInputMgr *input, NkEventHandler handler, void *user_data) {
    input->handler   = handler;
    input->user_data = user_data;
}

void nk_input_push(NkInputMgr *input, const NkEvent *event) {
    // Track pointer state
    switch (event->type) {
        case NK_EVENT_POINTER_MOTION:
            input->pointer_pos.x = event->motion.x;
            input->pointer_pos.y = event->motion.y;
            break;
        case NK_EVENT_POINTER_ENTER:
            input->pointer_inside = true;
            break;
        case NK_EVENT_POINTER_LEAVE:
            input->pointer_inside = false;
            break;
        default:
            break;
    }

    // Dispatch to handler
    if (input->handler) {
        input->handler(event, input->user_data);
    }
}

