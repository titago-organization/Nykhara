/**
 * @file ring_buffer.c
 * @brief Lock-free SPSC ring buffer — C11 atomics, acquire/release.
 */

#include "ring_buffer.h"

#include <stdlib.h>
#include <string.h>

// Round up to next power of 2
static size_t next_pow2(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

NkRingBuffer *nk_ring_create(size_t elem_size, size_t count) {
    size_t cap = next_pow2(count);
    NkRingBuffer *rb = calloc(1, sizeof(NkRingBuffer));
    if (!rb) return nullptr;

    rb->data      = calloc(cap, elem_size);
    if (!rb->data) { free(rb); return nullptr; }

    rb->capacity  = cap;
    rb->elem_size = elem_size;
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
    return rb;
}

void nk_ring_destroy(NkRingBuffer *rb) {
    if (rb) {
        free(rb->data);
        free(rb);
    }
}

bool nk_ring_push(NkRingBuffer *rb, const void *elem) {
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);

    // Full check: head is one slot behind tail (mod capacity)
    if (((head + 1) & (rb->capacity - 1)) == tail) {
        return false;
    }

    memcpy(rb->data + (head * rb->elem_size), elem, rb->elem_size);

    atomic_store_explicit(&rb->head, (head + 1) & (rb->capacity - 1), memory_order_release);
    return true;
}

bool nk_ring_pop(NkRingBuffer *rb, void *out_elem) {
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);

    if (tail == head) {
        return false; // Empty
    }

    memcpy(out_elem, rb->data + (tail * rb->elem_size), rb->elem_size);

    atomic_store_explicit(&rb->tail, (tail + 1) & (rb->capacity - 1), memory_order_release);
    return true;
}

size_t nk_ring_size(const NkRingBuffer *rb) {
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    return (head - tail) & (rb->capacity - 1);
}

bool nk_ring_empty(const NkRingBuffer *rb) {
    return nk_ring_size(rb) == 0;
}

