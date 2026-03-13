/**
 * @file anim_engine.c
 * @brief Animation engine — timeline scheduler with easing, chaining.
 */

#include "anim_engine.h"
#include "../core/nk_log.h"

#include <string.h>

void nk_anim_init(NkAnimEngine *eng) {
    memset(eng, 0, sizeof(*eng));
    eng->next_id = 1;
}

static NkAnimEntry *find_entry(NkAnimEngine *eng, uint32_t id) {
    for (uint32_t i = 0; i < eng->count; i++) {
        if (eng->entries[i].id == id) return &eng->entries[i];
    }
    return NULL;
}

uint32_t nk_anim_create(NkAnimEngine *eng, float from, float to,
                          float duration, float delay,
                          NkEasingFn easing,
                          void *target, NkAnimUpdateFn on_update,
                          NkAnimCompleteFn on_complete, void *user_data) {
    if (eng->count >= NK_ANIM_MAX_ACTIVE) {
        NK_LOG_WARN("Animation pool full");
        return 0;
    }

    uint32_t id = eng->next_id++;
    NkAnimEntry *e = &eng->entries[eng->count++];
    *e = (NkAnimEntry){
        .id          = id,
        .state       = NK_ANIM_RUNNING,
        .duration    = duration,
        .delay       = delay,
        .elapsed     = 0.0f,
        .easing      = easing ? easing : nk_ease_linear,
        .from        = from,
        .to          = to,
        .target      = target,
        .on_update   = on_update,
        .on_complete = on_complete,
        .user_data   = user_data,
        .chain_next  = 0,
    };
    return id;
}

void nk_anim_chain(NkAnimEngine *eng, uint32_t id, uint32_t next_id) {
    NkAnimEntry *e = find_entry(eng, id);
    if (e) e->chain_next = next_id;
}

void nk_anim_tick(NkAnimEngine *eng, float dt) {
    uint32_t i = 0;
    while (i < eng->count) {
        NkAnimEntry *e = &eng->entries[i];
        if (e->state != NK_ANIM_RUNNING) { i++; continue; }

        e->elapsed += dt;

        // Delay
        if (e->elapsed < e->delay) { i++; continue; }

        float t = (e->elapsed - e->delay) / e->duration;
        if (t > 1.0f) t = 1.0f;

        float eased = e->easing(t);
        float value = e->from + (e->to - e->from) * eased;

        if (e->on_update) {
            e->on_update(e->target, value, e->user_data);
        }

        if (t >= 1.0f) {
            e->state = NK_ANIM_FINISHED;
            if (e->on_complete) {
                e->on_complete(e->user_data);
            }
            // Trigger chain
            if (e->chain_next) {
                NkAnimEntry *next = find_entry(eng, e->chain_next);
                if (next && next->state == NK_ANIM_PAUSED) {
                    next->state = NK_ANIM_RUNNING;
                }
            }
            // Remove finished entry (swap with last)
            eng->entries[i] = eng->entries[--eng->count];
            continue; // Don't increment i
        }
        i++;
    }
}

void nk_anim_cancel(NkAnimEngine *eng, uint32_t id) {
    for (uint32_t i = 0; i < eng->count; i++) {
        if (eng->entries[i].id == id) {
            eng->entries[i] = eng->entries[--eng->count];
            return;
        }
    }
}

void nk_anim_cancel_all(NkAnimEngine *eng) {
    eng->count = 0;
}

