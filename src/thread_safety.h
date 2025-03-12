#ifndef JDAW_THREAD_SAFETY_H
#define JDAW_THREAD_SAFETY_H

#include <pthread.h>
#include <stdbool.h>

#define NUM_JDAW_THREADS 2
enum jdaw_thread {
    JDAW_THREAD_MAIN=0,
    JDAW_THREAD_DSP=1,
    JDAW_THREAD_OTHER=2
};

extern pthread_t MAIN_THREAD_ID;
extern pthread_t DSP_THREAD_ID;

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

#define DSP_THREAD_ONLY_WHEN_ACTIVE(name) \
    if (proj->playing && pthread_self() != DSP_THREAD_ID) { \
        fprintf(stderr, "Error: fn %s called outside DSP thread while proj playing", #name); \
	breakfn(); \
	exit(1);\
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
#define DSP_THREAD_ONLY_WHEN_ACTIVE(name)

#endif

#if defined(HAVE_THREAD_LOCAL)
    #define JDAW_THREAD_LOCAL thread_local
#elsif defined(HAVE__THREAD_LOCAL)
    #define JDAW_THREAD_LOCAL _Thread_local
#elsif defined(HAVE___THREAD)
    #define JDAW_THREAD_LOCAL __thread
#else
    #define JDAW_THREAD_LOCAL
#endif



const char *get_thread_name();
bool on_thread(enum jdaw_thread thread_index);

#endif
