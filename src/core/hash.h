/**
 * @file hash.h
 * @brief Fast hash functions for string interning & hash maps.
 */

#ifndef NK_HASH_H
#define NK_HASH_H

#include <stddef.h>
#include <stdint.h>

/**
 * FNV-1a 64-bit hash.
 */
uint64_t nk_hash_fnv1a(const void *data, size_t len);

/**
 * Hash a null-terminated string.
 */
uint64_t nk_hash_str(const char *str);

#endif // NK_HASH_H

