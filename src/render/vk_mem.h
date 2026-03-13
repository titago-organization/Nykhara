/**
 * @file vk_mem.h
 * @brief GPU Memory Allocator (GMA) — simple sub-allocator.
 */

#ifndef NK_VK_MEM_H
#define NK_VK_MEM_H

#include "vk_init.h"

// ── Block-based GPU memory pool ──

typedef struct NkGmaBlock {
    VkDeviceMemory  memory;
    VkDeviceSize    size;
    VkDeviceSize    offset;   // Next free offset
    uint32_t        mem_type;
    void           *mapped;   // Persistent-mapped pointer (host-visible only)
    struct NkGmaBlock *next;
} NkGmaBlock;

typedef struct NkGma {
    NkVkContext  *vk;
    NkGmaBlock   *blocks;
    VkDeviceSize  default_block_size;
} NkGma;

typedef struct NkGmaAlloc {
    NkGmaBlock   *block;
    VkDeviceSize  offset;
    VkDeviceSize  size;
} NkGmaAlloc;

/**
 * Initialize the GPU memory allocator.
 */
NkResult nk_gma_init(NkGma *gma, NkVkContext *vk, VkDeviceSize block_size);

/**
 * Allocate GPU memory.
 */
NkResult nk_gma_alloc(NkGma *gma, VkMemoryRequirements req,
                       VkMemoryPropertyFlags props, NkGmaAlloc *out);

/**
 * Get the mapped pointer for a host-visible allocation.
 */
void *nk_gma_map(NkGmaAlloc *alloc);

/**
 * Destroy all GPU memory.
 */
void nk_gma_destroy(NkGma *gma);

#endif // NK_VK_MEM_H

