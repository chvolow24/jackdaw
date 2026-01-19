enum log_level {
    FATAL,
    ERROR,
    WARN,
    INFO
};

void log_tmp(enum log_level level, char *fmt, ...);
void log_print_current_thread();
void log_printall();
void log_init();
void log_quit();
