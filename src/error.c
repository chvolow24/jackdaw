#include "error.h"
#include "log.h"
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void handle_error_signal(int signo)
{
    fprintf(stderr, "Jackdaw fatal error (%s). Dumping execution log:\n", strsignal(signo));
    log_printall();
    exit(1);
}

void error_register_signal_handlers()
{
    signal(SIGINT, handle_error_signal);
    signal(SIGSEGV, handle_error_signal);
    signal(SIGABRT, handle_error_signal);
}

void error_exit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}
