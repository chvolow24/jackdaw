/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "thread_safety.h"
#include "session_endpoint_ops.h"


static pthread_t THREAD_IDS[NUM_JDAW_THREADS];

static JDAW_THREAD_LOCAL pthread_t CURRENT_THREAD_ID = 0;
static JDAW_THREAD_LOCAL enum jdaw_thread CURRENT_THREAD_INDEX = -1;

/* Access on main thread only */
static int thread_active[NUM_JDAW_THREADS];
static _Atomic bool thread_req_cancel[NUM_JDAW_THREADS];

static const char *thread_names[NUM_JDAW_THREADS] = {
    "main",
    "dsp",
    "server",
    "playback",
    "instrument",
};

void set_thread_id(enum jdaw_thread index)
{
    pthread_t self = pthread_self();
    if (CURRENT_THREAD_ID != self) {
	CURRENT_THREAD_ID = self;
        THREAD_IDS[index] = self;
        CURRENT_THREAD_INDEX = index;
	log_tmp(LOG_DEBUG, "Set thread id for %s: %p\n", get_thread_name(index), self);
    }
}

pthread_t *get_thread_addr(enum jdaw_thread index)
{
    return &THREAD_IDS[index];
}

const char *get_thread_name(enum jdaw_thread thread)
{
    if (thread < NUM_JDAW_THREADS)
        return thread_names[thread];
    else return "other";
}

const char *get_current_thread_name()
{
    return get_thread_name(CURRENT_THREAD_INDEX);
}

bool on_thread(enum jdaw_thread thread_index)
{
    return CURRENT_THREAD_INDEX == thread_index;
}

enum jdaw_thread current_thread()
{
    if (CURRENT_THREAD_INDEX < 0) {
        log_tmp(LOG_WARN, "Current thread index not set (id %ld)\n", CURRENT_THREAD_ID);
        bool set = false;
        for (enum jdaw_thread t=0; t<NUM_JDAW_THREADS; t++) {
            if (CURRENT_THREAD_ID == THREAD_IDS[t]) {
                CURRENT_THREAD_INDEX = t;
                set = true;
                break;
            }
        }
        if (!set) {
            log_tmp(LOG_ERROR, "Current thread index not found (id %ld)\n", CURRENT_THREAD_ID);
            CURRENT_THREAD_INDEX = -1;
        }
    }
    return CURRENT_THREAD_INDEX;
    /* pthread_t id = CURRENT_THREAD_ID; */
}


void thread_set_active(enum jdaw_thread thread)
{
    MAIN_THREAD_ONLY(jdaw_thread_set_active);
    thread_active[thread] = true;
}

void thread_set_inactive(enum jdaw_thread thread)
{
    MAIN_THREAD_ONLY(jdaw_thread_set_active);
    thread_active[thread] = false;
}

bool thread_is_active(enum jdaw_thread thread)
{
    MAIN_THREAD_ONLY(jdaw_thread_set_active);
    return thread_active[thread];
}


void thread_start(enum jdaw_thread thread, pthread_attr_t *attr, void *(*threadfn)(void *), void *arg)
{
    MAIN_THREAD_ONLY(thread_start);
    if (thread_is_active(thread)) {
        log_tmp(LOG_WARN, "Call to activate an already-active thread (%s)\n", get_thread_name(thread));
        return;
    }
    atomic_store_explicit(&thread_req_cancel[thread], true, memory_order_relaxed);
    thread_set_active(thread);
    int ret = pthread_create(&THREAD_IDS[thread], attr, threadfn, arg);
    if (ret != 0) {
        log_tmp(LOG_ERROR, "pthread_create: %s\n", strerror(ret));
    }
}

void thread_cancel(enum jdaw_thread thread)
{
    MAIN_THREAD_ONLY(thread_cancel);
    if (!thread_is_active(thread)) {
        log_tmp(LOG_WARN, "Call to cancel an already-canceled thread (%s)\n", get_thread_name(thread));
        return;
    }
    atomic_store_explicit(&thread_req_cancel[thread], true, memory_order_relaxed);
    pthread_join(THREAD_IDS[thread], NULL);
    thread_set_inactive(thread);
    /* Leftover callbacks queued on the thread can be executed on main */
    session_run_thread_callbacks(thread);    
}
