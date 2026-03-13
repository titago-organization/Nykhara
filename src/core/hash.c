/**
 * @file hash.c
 * @brief FNV-1a hash implementation.
 */

#include "hash.h"

#include <string.h>

#define FNV_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV_PRIME        0x100000001b3ULL

uint64_t nk_hash_fnv1a(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint64_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

uint64_t nk_hash_str(const char *str) {
    return nk_hash_fnv1a(str, strlen(str));
}

