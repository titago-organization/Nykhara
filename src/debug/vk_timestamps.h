/**
 * @file vk_timestamps.h
 * @brief Vulkan GPU timestamp query pool for frame profiling.
 */

#ifndef NK_VK_TIMESTAMPS_H
#define NK_VK_TIMESTAMPS_H

#include "../render/vk_init.h"

#define NK_TS_MAX_QUERIES 64

typedef struct NkVkTimestamps {
    VkQueryPool  pool;
    uint32_t     count;
    uint64_t     results[NK_TS_MAX_QUERIES];
    float        timestamp_period;  // nanoseconds per tick
    bool         enabled;
} NkVkTimestamps;

/**
 * Create the timestamp query pool.
 */
NkResult nk_timestamps_init(NkVkTimestamps *ts, NkVkContext *vk);

/**
 * Reset queries at the start of a frame.
 */
void nk_timestamps_reset(NkVkTimestamps *ts, VkCommandBuffer cmd);

/**
 * Write a timestamp at the current pipeline stage.
 */
void nk_timestamps_write(NkVkTimestamps *ts, VkCommandBuffer cmd, VkPipelineStageFlagBits stage);

/**
 * Read back results (call after fence wait).
 */
void nk_timestamps_read(NkVkTimestamps *ts, NkVkContext *vk);

/**
 * Get elapsed time between two timestamps in milliseconds.
 */
float nk_timestamps_elapsed_ms(const NkVkTimestamps *ts, uint32_t start, uint32_t end);

/**
 * Destroy.
 */
void nk_timestamps_destroy(NkVkTimestamps *ts, NkVkContext *vk);

#endif // NK_VK_TIMESTAMPS_H

