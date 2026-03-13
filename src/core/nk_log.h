/**
 * @file nk_log.h
 * @brief Logging wrapper with categories.
 */

#ifndef NK_LOG_H
#define NK_LOG_H

typedef enum NkLogLevel {
    NK_LOG_TRACE,
    NK_LOG_DEBUG,
    NK_LOG_INFO,
    NK_LOG_WARN,
    NK_LOG_ERROR,
    NK_LOG_FATAL,
} NkLogLevel;

/**
 * Set minimum log level.
 */
void nk_log_set_level(NkLogLevel level);

/**
 * Internal log function — use macros below.
 */
void nk_log_write(NkLogLevel level, const char *file, int line, const char *fmt, ...);

#define NK_LOG_TRACE(...) nk_log_write(NK_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define NK_LOG_DEBUG(...) nk_log_write(NK_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define NK_LOG_INFO(...)  nk_log_write(NK_LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define NK_LOG_WARN(...)  nk_log_write(NK_LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define NK_LOG_ERROR(...) nk_log_write(NK_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define NK_LOG_FATAL(...) nk_log_write(NK_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif // NK_LOG_H

