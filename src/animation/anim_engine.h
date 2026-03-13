/**
 * @file anim_engine.h
 * @brief Timeline-driven animation scheduler with chaining, shape morphing.
 */

#ifndef NK_ANIM_ENGINE_H
#define NK_ANIM_ENGINE_H

#include <nykhara/types.h>
#include "easing.h"

#define NK_ANIM_MAX_ACTIVE 64

typedef enum NkAnimState {
    NK_ANIM_IDLE,
    NK_ANIM_RUNNING,
    NK_ANIM_PAUSED,
    NK_ANIM_FINISHED,
} NkAnimState;

typedef void (*NkAnimUpdateFn)(void *target, float value, void *user_data);
typedef void (*NkAnimCompleteFn)(void *user_data);

typedef struct NkAnimEntry {
    uint32_t         id;
    NkAnimState      state;
    float            duration;     // seconds
    float            delay;        // seconds
    float            elapsed;      // seconds
    NkEasingFn       easing;
    float            from;
    float            to;
    void            *target;
    NkAnimUpdateFn   on_update;
    NkAnimCompleteFn on_complete;
    void            *user_data;
    uint32_t         chain_next;   // ID of animation to trigger on completion (0 = none)
} NkAnimEntry;

typedef struct NkAnimEngine {
    NkAnimEntry   entries[NK_ANIM_MAX_ACTIVE];
    uint32_t      count;
    uint32_t      next_id;
} NkAnimEngine;

/**
 * Initialize the animation engine.
 */
void nk_anim_init(NkAnimEngine *eng);

/**
 * Create a new animation. Returns its ID.
 */
uint32_t nk_anim_create(NkAnimEngine *eng, float from, float to,
                          float duration, float delay,
                          NkEasingFn easing,
                          void *target, NkAnimUpdateFn on_update,
                          NkAnimCompleteFn on_complete, void *user_data);

/**
 * Chain: when anim `id` completes, start `next_id`.
 */
void nk_anim_chain(NkAnimEngine *eng, uint32_t id, uint32_t next_id);

/**
 * Tick all active animations. Call once per frame with delta time.
 */
void nk_anim_tick(NkAnimEngine *eng, float dt);

/**
 * Cancel an animation.
 */
void nk_anim_cancel(NkAnimEngine *eng, uint32_t id);

/**
 * Cancel all animations.
 */
void nk_anim_cancel_all(NkAnimEngine *eng);

#endif // NK_ANIM_ENGINE_H

