#include "error.h"
#include "log.h"
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* static struct sigaction old_sigsegv, old_sigabrt, old_sigint; */

/* static void handle_error_signal(int signo) */
/* { */
/*     fprintf(stderr, "appname fatal error (%s). Dumping execution log:\n", strsignal(signo)); */

    
/*     struct sigaction *old = NULL; */
/*     switch (signo) { */
/*         case SIGSEGV: old = &old_sigsegv; break; */
/*         case SIGABRT: old = &old_sigabrt; break; */
/*         case SIGINT:  old = &old_sigint;  break; */
/*     } */
/*     if (old && old->sa_handler && old->sa_handler != SIG_IGN && old->sa_handler != SIG_DFL) { */
/* 	fprintf(stderr, "REDIRECTING SIGNAL to old handler\n"); */
/*         old->sa_handler(signo); */
/*     } */

/*     log_printall(); */
    
/*     /\* } else { *\/ */
/*     /\*     signal(signo, SIG_DFL); *\/ */
/*     /\*     raise(signo); *\/ */
/*     /\* } *\/ */
/* } */

/* void error_register_signal_handlers() */
/* { */
/*     struct sigaction sa = {0}; */
/*     sa.sa_handler = handle_error_signal; */
/*     sigemptyset(&sa.sa_mask); */
    
/*     sigaction(SIGINT, &sa, &old_sigint); */
/*     sigaction(SIGSEGV, &sa, &old_sigsegv); */
/*     sigaction(SIGABRT, &sa, &old_sigabrt); */
/* } */

/* static struct sigaction old_sigsegv, old_sigabrt, old_sigint; */

/* static void handle_error_signal(int signo) */
/* { */
/*     fprintf(stderr, "Jackdaw fatal error (%s). Dumping execution log:\n", strsignal(signo)); */
/*     log_printall(); */
/*     signal(signo, NULL); /\* re-register default handler *\/ */
/*     raise(signo); */
/* } */

/* void error_register_signal_handlers() */
/* { */
/*     signal(SIGINT, handle_error_signal); */
/*     signal(SIGSEGV, handle_error_signal); */
/*     signal(SIGABRT, handle_error_signal); */
/* } */

void error_exit(const char *fmt, ...)
{
    log_printall();
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}
