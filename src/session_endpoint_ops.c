/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "session_endpoint_ops.h"
#include "endpoint.h"
#include "log.h"
#include "spsc_lfqueue.h"
#include "timeline.h"

void session_enqueue_callback(enum jdaw_thread for_thread, struct queued_cb cb)
{
    Session *session = session_get();
    enum jdaw_thread writer = current_thread();
    LFQueue *q = &session->queued_ops.queued_callbacks_v2[for_thread][writer];
    int ret = lfqueue_try_enqueue(q, &cb, 1);
    if (ret != LFQUEUE_SUCCESS) {
        log_tmp(LOG_WARN, "Error enqueueing ep \"%s\" cb on thread %s from %s: %s\n", cb.ep->local_id, get_thread_name(for_thread), get_current_thread_name(), lfqueue_get_errstr(ret));
    }
}

void session_run_thread_callbacks(enum jdaw_thread thread)
{
    Session *session = session_get();
    LFQueue *arr = session->queued_ops.queued_callbacks_v2[thread];
    struct queued_cb cbs[MAX_CBS_PER_QUEUE * NUM_EP_WRITER_THREADS] = {0};
    int num_cbs = 0;
    for (enum jdaw_thread t=0; t<NUM_EP_WRITER_THREADS; t++) {
        LFQueue *queue = arr + t;
        int ret;
        while ((ret = lfqueue_try_dequeue(queue, cbs + num_cbs, 1)) == LFQUEUE_SUCCESS) {
            num_cbs++;
        }
    }
    /* Work backwards to dedupe */
    int num_seen = 0;
    struct queued_cb seen[num_cbs];
    for (int i=num_cbs - 1; i>=0; i--) {
        struct queued_cb cb = cbs[i];
        bool dupe = false;
        for (int j=0; j<num_seen; j++) {
            if (memcmp(&cb, &seen[j], sizeof(cb)) == 0) {
                dupe = true;
                break;
            }
        }
        if (!dupe) {
            seen[num_seen] = cb;
            num_seen++;
        }
    }
    /* After dedupe, run original order */
    for (int i=num_seen - 1; i>=0; i--) {
        seen[i].cb(seen[i].ep);
    }    
}


void session_flush_callbacks()
{
    MAIN_THREAD_ONLY(session_flush_callbacks);
    for (enum jdaw_thread t=0; t<NUM_JDAW_THREADS; t++) {
        if (thread_is_active(t) && t != JDAW_THREAD_MAIN) {
            log_tmp(LOG_ERROR, "Attempting to %s callbacks while still active (from main)\n", get_thread_name(t));
            return;
        }
        session_run_thread_callbacks(t);
    }
}


