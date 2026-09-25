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

static void add_ongoing_change_cb(Endpoint *ep)
{
    Session *session = session_get();
    int index = session->queued_ops.num_ongoing_changes[current_thread()]++;
    session->queued_ops.ongoing_changes[current_thread()][index] = ep;
}
static void clear_ongoing_changes_cb(Endpoint *ep)
{
    Session *session = session_get();
    for (int i=0; i<session->queued_ops.num_ongoing_changes[current_thread()]; i++) {
        endpoint_stop_continuous_change(session->queued_ops.ongoing_changes[current_thread()][i]);
    }
    session->queued_ops.num_ongoing_changes[current_thread()] = 0;
}

void session_add_ongoing_change(Endpoint *ep, enum jdaw_thread thread)
{
    MAIN_THREAD_ONLY(session_add_ongoing_change);
    struct queued_cb cb;
    cb.ep = ep;
    cb.cb = add_ongoing_change_cb;
    session_enqueue_callback(thread, cb);
}


int session_do_ongoing_changes(enum jdaw_thread thread)
{
    Session *session = session_get();
    int incr = 0;
    for (int i=0; i<session->queued_ops.num_ongoing_changes[thread]; i++) {
        Endpoint *ep = session->queued_ops.ongoing_changes[thread][i];
        if (ep->do_auto_incr) {
            endpoint_continuous_change_do_incr(ep);
            incr++;
        }
    }
    return incr;
}

void session_clear_ongoing_changes(enum jdaw_thread thread)
{
    MAIN_THREAD_ONLY(session_add_ongoing_change);
    struct queued_cb cb;
    cb.ep = NULL;
    cb.cb = clear_ongoing_changes_cb;
    session_enqueue_callback(thread, cb);
}

void session_clear_all_ongoing_changes()
{
    for (enum jdaw_thread t=0; t<NUM_JDAW_THREADS; t++) {
        session_clear_ongoing_changes(t);
    }
}


void session_enqueue_callback(enum jdaw_thread for_thread, struct queued_cb cb)
{
    Session *session = session_get();
    if (on_thread(JDAW_THREAD_MAIN) && !thread_is_active(for_thread)) {
        cb.cb(cb.ep);
        return;
    }
    enum jdaw_thread writer = current_thread();
    LFQueue *q = &session->queued_ops.queued_callbacks_v2[for_thread][writer];
    int ret = lfqueue_try_enqueue(q, &cb, 1);
    if (ret != LFQUEUE_SUCCESS) {
        TESTBREAK;
        log_tmp(LOG_WARN, "Error enqueueing ep \"%s\" cb on thread %s from %s: %s\n", cb.ep ? cb.ep->local_id : "(no ep)", get_thread_name(for_thread), get_current_thread_name(), lfqueue_get_errstr(ret));
    }
}

int session_run_thread_callbacks(enum jdaw_thread thread)
{
    /* if (thread == JDAW_THREAD_MAIN) fprintf(stderr, "Running callbacks on main thread\n"); */
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
    if (thread == JDAW_THREAD_MAIN && num_cbs > 0) fprintf(stderr, "....found %d\n", num_cbs);
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
    return num_seen;
}


void session_clear_all_queued_callbacks()
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


