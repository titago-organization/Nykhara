/**
 * @file vk_mem.c
 * @brief GPU Memory Allocator — block-based sub-allocator.
 */

#include "vk_mem.h"
#include "../core/nk_log.h"

#include <stdlib.h>
#include <string.h>

static NkGmaBlock *gma_alloc_block(NkGma *gma, VkDeviceSize size, uint32_t mem_type, bool map) {
    NkGmaBlock *block = calloc(1, sizeof(NkGmaBlock));
    if (!block) return NULL;

    VkMemoryAllocateInfo ai = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = size,
        .memoryTypeIndex = mem_type,
    };

    VkResult vr = vkAllocateMemory(gma->vk->device, &ai, NULL, &block->memory);
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkAllocateMemory failed: %d (size=%zu, type=%u)", vr, (size_t)size, mem_type);
        free(block);
        return NULL;
    }

    block->size     = size;
    block->offset   = 0;
    block->mem_type = mem_type;
    block->mapped   = NULL;
    block->next     = NULL;

    if (map) {
        vkMapMemory(gma->vk->device, block->memory, 0, VK_WHOLE_SIZE, 0, &block->mapped);
    }

    return block;
}

NkResult nk_gma_init(NkGma *gma, NkVkContext *vk, VkDeviceSize block_size) {
    memset(gma, 0, sizeof(*gma));
    gma->vk                 = vk;
    gma->blocks             = NULL;
    gma->default_block_size = block_size > 0 ? block_size : (64 * 1024 * 1024); // 64 MiB
    return NK_SUCCESS;
}

NkResult nk_gma_alloc(NkGma *gma, VkMemoryRequirements req,
                       VkMemoryPropertyFlags props, NkGmaAlloc *out) {
    memset(out, 0, sizeof(*out));

    int32_t mem_type = nk_vk_find_memory_type(gma->vk, req.memoryTypeBits, props);
    if (mem_type < 0) {
        NK_LOG_ERROR("No suitable memory type found");
        return NK_ERROR_VULKAN;
    }

    bool host_visible = (props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

    // Try to sub-allocate from existing block
    for (NkGmaBlock *b = gma->blocks; b; b = b->next) {
        if (b->mem_type != (uint32_t)mem_type) continue;

        VkDeviceSize aligned_offset = (b->offset + req.alignment - 1) & ~(req.alignment - 1);
        if (aligned_offset + req.size <= b->size) {
            out->block  = b;
            out->offset = aligned_offset;
            out->size   = req.size;
            b->offset   = aligned_offset + req.size;
            return NK_SUCCESS;
        }
    }

    // Allocate new block
    VkDeviceSize block_size = gma->default_block_size;
    if (req.size > block_size) block_size = req.size;

    NkGmaBlock *block = gma_alloc_block(gma, block_size, (uint32_t)mem_type, host_visible);
    if (!block) return NK_ERROR_OUT_OF_MEMORY;

    // Prepend to list
    block->next = gma->blocks;
    gma->blocks = block;

    out->block  = block;
    out->offset = 0;
    out->size   = req.size;
    block->offset = req.size;
    return NK_SUCCESS;
}

void *nk_gma_map(NkGmaAlloc *alloc) {
    if (!alloc->block || !alloc->block->mapped) return NULL;
    return (uint8_t *)alloc->block->mapped + alloc->offset;
}

void nk_gma_destroy(NkGma *gma) {
    NkGmaBlock *b = gma->blocks;
    while (b) {
        NkGmaBlock *next = b->next;
        if (b->mapped) {
            vkUnmapMemory(gma->vk->device, b->memory);
        }
        vkFreeMemory(gma->vk->device, b->memory, NULL);
        free(b);
        b = next;
    }
    gma->blocks = NULL;
}

