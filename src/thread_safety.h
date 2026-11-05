/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
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
#include <stdint.h>
#include "error.h"
#include "test.h"

/* Number of threads on which endpoint ops can occur;
   restricted to main, dsp, and playback threads */
/* #define NUM_JDAW_THREADS 3 */
enum jdaw_thread {
    JDAW_THREAD_MAIN,
    JDAW_THREAD_DSP,
    JDAW_THREAD_API_SERVER,
    JDAW_THREAD_INSTRUMENT,
    /* Above threads can call endpoint_write; below cannot */
    JDAW_THREAD_PLAYBACK,
    NUM_JDAW_THREADS
};

#define NUM_EP_WRITER_THREADS (JDAW_THREAD_PLAYBACK)

/* extern pthread_t MAIN_THREAD_ID; */
/* extern pthread_t DSP_THREAD_ID; */
#define BT_AND_EXIT(msg) \
    char bt[2048] = {0};                                                \
    snprint_backtrace(bt, 2048);                                        \
    error_exit("\nBacktrace:\n%s\n" msg "\n\n", bt);                   \
    

#ifdef TESTBUILD
#define RESTRICT_NOT_MAIN(name)                                         \
    if (on_thread(JDAW_THREAD_MAIN)) {					\
        BT_AND_EXIT("Error: fn " #name " called in main thread");      \
    }

#define RESTRICT_NOT_DSP(name)                                       \
    if (on_thread(JDAW_THREAD_DSP)) {				     \
	BT_AND_EXIT("Error: fn " #name " called in DSP thread");    \
    }

#define DSP_THREAD_ONLY(name)		   \
    if (!on_thread(JDAW_THREAD_DSP)) {					\
        BT_AND_EXIT("Error: fn " #name " called outside DSP thread");  \
    }

#define DSP_THREAD_ONLY_WHEN_ACTIVE(name) \
    if (aldr(&session->playback.playing) && !on_thread(JDAW_THREAD_DSP)) { \
        BT_AND_EXIT("Error: fn " #name " called outside DSP thread while proj playing"); \
    }

#define MAIN_THREAD_ONLY(name)                                          \
    if (!on_thread(JDAW_THREAD_MAIN)) {					\
	BT_AND_EXIT("Error: fn " #name  " called outside main thread");  \
    }

#define RESTRICT_THREAD(name, condition)        \
    if (!(condition)) {                         \
        BT_AND_EXIT("Error: thread condition failed: " #name "");      \
    }       \

#else

#define RESTRICT_NOT_MAIN(name)
#define RESTRICT_NOT_DSP(name)
#define DSP_THREAD_ONLY(name)
#define MAIN_THREAD_ONLY(name)
#define DSP_THREAD_ONLY_WHEN_ACTIVE(name)
#define RESTRICT_THREAD(name, condition)

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

/* Main thread only */
void thread_set_active(enum jdaw_thread thread);
/* Main thread only */
void thread_set_inactive(enum jdaw_thread thread);
/* Main thread only */
bool thread_is_active(enum jdaw_thread thread);


/* Main thread only; entrypoint for all loop threads */
void thread_start(enum jdaw_thread thread, pthread_attr_t *attr, void *(*threadfn)(void *), void *arg);
/* Main thread only; exit point for all loop threads (includes join) */
void thread_cancel(enum jdaw_thread thread);
/* Loop condition in thread */
bool thread_not_canceled();

_Atomic bool *thread_get_cancellation_bool(enum jdaw_thread thread);
uint32_t thread_epoch(enum jdaw_thread thread);
void thread_increment_epoch();

#endif
