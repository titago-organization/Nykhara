/**
 * @file vk_timestamps.c
 * @brief Vulkan GPU timestamp profiling implementation.
 */

#include "vk_timestamps.h"
#include "../core/nk_log.h"

#include <string.h>

NkResult nk_timestamps_init(NkVkTimestamps *ts, NkVkContext *vk) {
    memset(ts, 0, sizeof(*ts));

    if (vk->device_props.limits.timestampPeriod == 0) {
        NK_LOG_WARN("GPU does not support timestamps");
        ts->enabled = false;
        return NK_SUCCESS;
    }

    ts->timestamp_period = vk->device_props.limits.timestampPeriod;

    VkQueryPoolCreateInfo ci = {
        .sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .queryType  = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = NK_TS_MAX_QUERIES,
    };

    VkResult vr = vkCreateQueryPool(vk->device, &ci, NULL, &ts->pool);
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkCreateQueryPool failed: %d", vr);
        return NK_ERROR_VULKAN;
    }

    ts->enabled = true;
    NK_LOG_DEBUG("Vulkan timestamps initialized (period=%.2f ns)", ts->timestamp_period);
    return NK_SUCCESS;
}

void nk_timestamps_reset(NkVkTimestamps *ts, VkCommandBuffer cmd) {
    if (!ts->enabled) return;
    vkCmdResetQueryPool(cmd, ts->pool, 0, NK_TS_MAX_QUERIES);
    ts->count = 0;
}

void nk_timestamps_write(NkVkTimestamps *ts, VkCommandBuffer cmd, VkPipelineStageFlagBits stage) {
    if (!ts->enabled || ts->count >= NK_TS_MAX_QUERIES) return;
    vkCmdWriteTimestamp(cmd, stage, ts->pool, ts->count);
    ts->count++;
}

void nk_timestamps_read(NkVkTimestamps *ts, NkVkContext *vk) {
    if (!ts->enabled || ts->count == 0) return;
    vkGetQueryPoolResults(vk->device, ts->pool, 0, ts->count,
                          sizeof(ts->results), ts->results,
                          sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
}

float nk_timestamps_elapsed_ms(const NkVkTimestamps *ts, uint32_t start, uint32_t end) {
    if (!ts->enabled || start >= ts->count || end >= ts->count) return 0.0f;
    uint64_t diff = ts->results[end] - ts->results[start];
    return (float)diff * ts->timestamp_period / 1e6f;
}

void nk_timestamps_destroy(NkVkTimestamps *ts, NkVkContext *vk) {
    if (ts->pool) {
        vkDestroyQueryPool(vk->device, ts->pool, NULL);
    }
    memset(ts, 0, sizeof(*ts));
}

