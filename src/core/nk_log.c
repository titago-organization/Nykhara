/**
 * @file nk_log.c
 * @brief Simple logging implementation — stderr with ANSI colors.
 */

#include "nk_log.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static NkLogLevel g_min_level = NK_LOG_DEBUG;

static const char *level_names[] = {
    [NK_LOG_TRACE] = "TRACE",
    [NK_LOG_DEBUG] = "DEBUG",
    [NK_LOG_INFO]  = "INFO ",
    [NK_LOG_WARN]  = "WARN ",
    [NK_LOG_ERROR] = "ERROR",
    [NK_LOG_FATAL] = "FATAL",
};

static const char *level_colors[] = {
    [NK_LOG_TRACE] = "\033[37m",    // white
    [NK_LOG_DEBUG] = "\033[36m",    // cyan
    [NK_LOG_INFO]  = "\033[32m",    // green
    [NK_LOG_WARN]  = "\033[33m",    // yellow
    [NK_LOG_ERROR] = "\033[31m",    // red
    [NK_LOG_FATAL] = "\033[35m",    // magenta
};

void nk_log_set_level(NkLogLevel level) {
    g_min_level = level;
}

void nk_log_write(NkLogLevel level, const char *file, int line, const char *fmt, ...) {
    if (level < g_min_level) return;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double sec = (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;

    fprintf(stderr, "%s[%8.3f] %s %s:%d: \033[0m",
            level_colors[level], sec, level_names[level], file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

