/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    thread_safety.h

    * define standard jackdaw execution thread identifiers
    * test-only guardrails for restricted functions
*****************************************************************************************************************/


#ifndef JDAW_THREAD_SAFETY_H
#define JDAW_THREAD_SAFETY_H

#include <pthread.h>
#include <stdbool.h>

/* Number of threads on which endpoint ops can occur;
   restricted to main, dsp, and playback threads */
/* #define NUM_JDAW_THREADS 3 */
enum jdaw_thread {
    JDAW_THREAD_MAIN,
    JDAW_THREAD_DSP,
    JDAW_THREAD_PLAYBACK,
    NUM_JDAW_THREADS
};

/* extern pthread_t MAIN_THREAD_ID; */
/* extern pthread_t DSP_THREAD_ID; */

#ifdef TESTBUILD
#define RESTRICT_NOT_MAIN(name) \
    if (on_thread(JDAW_THREAD_MAIN)) {					\
	fprintf(stderr, "Error: fn %s called in main thread", #name);	\
	exit(1);							\
    }

#define RESTRICT_NOT_DSP(name) \
    if (on_thread(JDAW_THREAD_DSP)) {				     \
	fprintf(stderr, "Error: fn %s called in DSP thread", #name); \
	exit(1); \
    }

#define DSP_THREAD_ONLY(name)		   \
    if (!on_thread(JDAW_THREAD_DSP)) {					\
	fprintf(stderr, "Error: fn %s called outside DSP thread", #name); \
	exit(1); \
    }

#define DSP_THREAD_ONLY_WHEN_ACTIVE(name) \
    if (session->playback.playing && !on_thread(JDAW_THREAD_DSP)) {	\
        fprintf(stderr, "Error: fn %s called outside DSP thread while proj playing", #name); \
	breakfn(); \
	exit(1);\
    }

#define MAIN_THREAD_ONLY(name)		   \
    if (!on_thread(JDAW_THREAD_MAIN)) {					\
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

/* #if defined(HAVE_THREAD_LOCAL) */
/*     #define JDAW_THREAD_LOCAL thread_local */
/* #elif defined(HAVE__THREAD_LOCAL) */
/*     #define JDAW_THREAD_LOCAL _Thread_local */
/* #elif defined(HAVE___THREAD) */
/*     #define JDAW_THREAD_LOCAL __thread */
/* #else */
/*     #define JDAW_THREAD_LOCAL */
/* #endif */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
    /* C11 with threads support */
    #define JDAW_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) || defined(__clang__)
    /* GCC, Clang, or compatible */
    #define JDAW_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
    /* Microsoft Visual C++ */
    #define JDAW_THREAD_LOCAL __declspec(thread)
#else
    /* No thread-local support */
    #define JDAW_THREAD_LOCAL
    #warning "No thread-local storage support detected"
#endif

void set_thread_id(enum jdaw_thread index);
pthread_t *get_thread_addr(enum jdaw_thread index);
enum jdaw_thread current_thread();
const char *get_current_thread_name();
const char *get_thread_name(enum jdaw_thread thread);
bool on_thread(enum jdaw_thread thread_index);
#endif
