/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "thread_safety.h"


static pthread_t THREAD_IDS[NUM_JDAW_THREADS];
/* static pthread_t MAIN_THREAD_ID = 0; */
/* static pthread_t DSP_THREAD_ID = 0; */
/* static pthread_t PLAYBACK_THREAD_ID = 0; */
/* static pthread_t INSTRUMENT_THREAD_ID = 0; */

static JDAW_THREAD_LOCAL pthread_t CURRENT_THREAD_ID = 0;
static JDAW_THREAD_LOCAL enum jdaw_thread CURRENT_THREAD_INDEX = -1;

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
