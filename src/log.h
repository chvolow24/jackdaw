#ifndef JDAW_LOG_H
#define JDAW_LOG_H

#include <stdarg.h>

enum log_level {
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

/* Write to a thread-specific log in tmp directory */
void log_tmp(enum log_level level, char *fmt, ...);

/* Same as log_tmp but takes va_list instead of ... */
void log_tmp_v(enum log_level level, const char *fmt, va_list ap);
void log_print_current_thread();
void log_printall();
void log_init();
void log_quit();

void log_print_last_session();


#endif
