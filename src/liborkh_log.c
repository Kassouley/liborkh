#include <stdio.h>
#include <stdarg.h>
#include "liborkh_log.h"

// Log function with file/line/function info
void liborkh_log(log_level_t level, const char* file, const char* func, int line, const char* fmt, ...) {
    // Log level string & color
    switch(level) {
        case LOG_INFO:  fprintf(stderr, "%s[INFO]%s liborkh : ", COLOR_GREEN, COLOR_RESET); break; // green
        case LOG_WARN:  fprintf(stderr, "%s[WARN]%s liborkh : ", COLOR_YELLOW, COLOR_RESET); break; // yellow
        case LOG_ERROR: fprintf(stderr, "%s[ERROR]%s liborkh : %s:%d %s(): ", COLOR_RED, COLOR_RESET, file, line, func); break; // red
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}