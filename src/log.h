#ifndef JDAW_LOG_H
#define JDAW_LOG_H

enum log_level {
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

void log_tmp(enum log_level level, char *fmt, ...);
void log_print_current_thread();
void log_printall();
void log_init();
void log_quit();


#endif
