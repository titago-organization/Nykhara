/**
 * @file input_mgr.h
 * @brief Central input event dispatcher.
 */

#ifndef NK_INPUT_MGR_H
#define NK_INPUT_MGR_H

#include <nykhara/types.h>

typedef enum NkEventType {
    NK_EVENT_NONE = 0,
    // Pointer
    NK_EVENT_POINTER_MOTION,
    NK_EVENT_POINTER_BUTTON,
    NK_EVENT_POINTER_SCROLL,
    NK_EVENT_POINTER_ENTER,
    NK_EVENT_POINTER_LEAVE,
    // Keyboard
    NK_EVENT_KEY_PRESS,
    NK_EVENT_KEY_RELEASE,
    NK_EVENT_KEY_REPEAT,
    // Window
    NK_EVENT_RESIZE,
    NK_EVENT_CLOSE,
    NK_EVENT_FOCUS,
    NK_EVENT_BLUR,
} NkEventType;

typedef struct NkEvent {
    NkEventType type;
    union {
        struct { float x, y; }                          motion;
        struct { uint32_t button; bool pressed; }       button;
        struct { float dx, dy; }                        scroll;
        struct { uint32_t keycode; uint32_t sym; }      key;
        struct { uint32_t width, height; }              resize;
    };
} NkEvent;

typedef void (*NkEventHandler)(const NkEvent *event, void *user_data);

typedef struct NkInputMgr {
    NkEventHandler  handler;
    void           *user_data;
    NkVec2          pointer_pos;
    bool            pointer_inside;
} NkInputMgr;

/**
 * Initialize the input manager.
 */
void nk_input_init(NkInputMgr *input);

/**
 * Set the event handler callback.
 */
void nk_input_set_handler(NkInputMgr *input, NkEventHandler handler, void *user_data);

/**
 * Push an event (called by platform layer).
 */
void nk_input_push(NkInputMgr *input, const NkEvent *event);

#endif // NK_INPUT_MGR_H

