/**
 * @file arena.h
 * @brief Arena (bump) allocator — fast, bulk-freed allocations.
 */

#ifndef NK_ARENA_H
#define NK_ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define NK_ARENA_DEFAULT_BLOCK_SIZE (64 * 1024) // 64 KiB

typedef struct NkArenaBlock {
    struct NkArenaBlock *next;
    size_t               capacity;
    size_t               used;
    alignas(16) uint8_t  data[];
} NkArenaBlock;

typedef struct NkArena {
    NkArenaBlock *current;
    NkArenaBlock *first;
    size_t        block_size;
} NkArena;

/**
 * Initialize an arena with the given block size (0 = default 64K).
 */
void  nk_arena_init(NkArena *a, size_t block_size);

/**
 * Allocate `size` bytes with `align` alignment.
 */
void *nk_arena_alloc(NkArena *a, size_t size, size_t align);

/**
 * Convenience: allocate zeroed memory.
 */
void *nk_arena_calloc(NkArena *a, size_t count, size_t elem_size);

/**
 * Free ALL memory in the arena at once.
 */
void  nk_arena_reset(NkArena *a);

/**
 * Destroy the arena and release pages back to OS.
 */
void  nk_arena_destroy(NkArena *a);

/**
 * Helper macro: allocate a struct from arena.
 */
#define nk_arena_new(arena, T) ((T *)nk_arena_alloc((arena), sizeof(T), _Alignof(T)))

#endif // NK_ARENA_H

