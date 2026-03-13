/**
 * @file ring_buffer.h
 * @brief Lock-free single-producer single-consumer (SPSC) ring buffer.
 *
 * Uses C11 atomics with acquire/release ordering.
 * Fixed-size, power-of-two capacity.
 */

#ifndef NK_RING_BUFFER_H
#define NK_RING_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

typedef struct NkRingBuffer {
    uint8_t          *data;
    size_t            capacity;   // Must be power of 2
    size_t            elem_size;
    atomic_size_t     head;       // Written by producer
    atomic_size_t     tail;       // Written by consumer
} NkRingBuffer;

/**
 * Create a ring buffer.
 * @param elem_size  Size of each element in bytes.
 * @param count      Number of slots (will be rounded up to power of 2).
 */
NkRingBuffer *nk_ring_create(size_t elem_size, size_t count);

/**
 * Destroy and free the ring buffer.
 */
void nk_ring_destroy(NkRingBuffer *rb);

/**
 * Push one element (producer side). Returns false if full.
 */
bool nk_ring_push(NkRingBuffer *rb, const void *elem);

/**
 * Pop one element (consumer side). Returns false if empty.
 */
bool nk_ring_pop(NkRingBuffer *rb, void *out_elem);

/**
 * Number of elements currently in the buffer.
 */
size_t nk_ring_size(const NkRingBuffer *rb);

/**
 * Is the buffer empty?
 */
bool nk_ring_empty(const NkRingBuffer *rb);

#endif // NK_RING_BUFFER_H

