/**
 * @file arena.c
 * @brief Arena allocator implementation — mmap-backed bump allocator.
 */

#include "arena.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// Add other POSIX OS support:
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
#elif !defined(MAP_ANONYMOUS)
#  define MAP_ANONYMOUS 0x1000
#endif

// ── Internal ──

static NkArenaBlock *arena_alloc_block(size_t capacity) {
    size_t total = sizeof(NkArenaBlock) + capacity;
    NkArenaBlock *block = mmap(NULL, total, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (block == MAP_FAILED) return NULL;
    block->next     = NULL;
    block->capacity = capacity;
    block->used     = 0;
    return block;
}

static void arena_free_block(NkArenaBlock *block) {
    if (block) {
        size_t total = sizeof(NkArenaBlock) + block->capacity;
        munmap(block, total);
    }
}

// ── Public API ──

void nk_arena_init(NkArena *a, size_t block_size) {
    a->block_size = block_size > 0 ? block_size : NK_ARENA_DEFAULT_BLOCK_SIZE;
    a->first      = NULL;
    a->current    = NULL;
}

void *nk_arena_alloc(NkArena *a, size_t size, size_t align) {
    if (align < 1) align = 1;

    if (a->current) {
        size_t offset = (a->current->used + align - 1) & ~(align - 1);
        if (offset + size <= a->current->capacity) {
            void *ptr = a->current->data + offset;
            a->current->used = offset + size;
            return ptr;
        }
    }

    size_t cap = a->block_size;
    if (size + align > cap) cap = size + align;

    NkArenaBlock *block = arena_alloc_block(cap);
    if (!block) return NULL;

    block->next = NULL;
    if (a->current) {
        a->current->next = block;
    } else {
        a->first = block;
    }
    a->current = block;

    size_t offset = (block->used + align - 1) & ~(align - 1);
    void *ptr = block->data + offset;
    block->used = offset + size;
    return ptr;
}

void *nk_arena_calloc(NkArena *a, size_t count, size_t elem_size) {
    size_t total = count * elem_size;
    void *ptr = nk_arena_alloc(a, total, _Alignof(max_align_t));
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void nk_arena_reset(NkArena *a) {
    for (NkArenaBlock *b = a->first; b; b = b->next) {
        b->used = 0;
    }
    a->current = a->first;
}

void nk_arena_destroy(NkArena *a) {
    NkArenaBlock *b = a->first;
    while (b) {
        NkArenaBlock *next = b->next;
        arena_free_block(b);
        b = next;
    }
    a->first   = NULL;
    a->current = NULL;
}
