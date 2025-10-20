#ifndef LIBORKH_LOG_H
#define LIBORKH_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"

typedef enum { LOG_INFO, LOG_WARN, LOG_ERROR } log_level_t;

void liborkh_log(log_level_t level, const char* file, const char* func, int line, const char* fmt, ...);

// Macro to automatically fill file, func, line
#define liborkh_log_err(fmt, ...)  liborkh_log(LOG_ERROR, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define liborkh_log_warn(fmt, ...) liborkh_log(LOG_WARN, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define liborkh_log_info(fmt, ...) liborkh_log(LOG_INFO, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#endif // LIBORKH_LOG_H