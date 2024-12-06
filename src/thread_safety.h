#ifndef JDAW_THREAD_SAFETY_H
#define JDAW_THREAD_SAFETY_H

#include <pthread.h>

#ifdef TESTBUILD
#define RESTRICT_NOT_MAIN(name) \
    if (pthread_self() == MAIN_THREAD_ID) { \
	fprintf(stderr, "Error: fn %s called in main thread", #name); \
	exit(1); \
    }

#define RESTRICT_NOT_DSP(name) \
    if (pthread_self() == DSP_THREAD_ID) { \
	fprintf(stderr, "Error: fn %s called in DSP thread", #name); \
	exit(1); \
    }

#define DSP_THREAD_ONLY(name)		   \
    if (pthread_self() != DSP_THREAD_ID) { \
	fprintf(stderr, "Error: fn %s called outside DSP thread", #name); \
	exit(1); \
    }

#define MAIN_THREAD_ONLY(name)		   \
    if (pthread_self() != MAIN_THREAD_ID) { \
	fprintf(stderr, "Error: fn %s called outside main thread", #name); \
	exit(1); \
    }

#else

#define RESTRICT_NOT_MAIN(name)
#define RESTRICT_NOT_DSP(name)
#define DSP_THREAD_ONLY(name)
#define MAIN_THREAD_ONLY(name)

#endif


const char *get_thread_name();

#endif
