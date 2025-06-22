/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "session_endpoint_ops.h"
#include "endpoint.h"
#include "timeline.h"

/* CALLBACKS */

/* Returns 0 on success, 1 if maximum number of callbacks reached */
int session_queue_val_change(Session *session, Endpoint *ep, Value new_val, bool run_gui_cb)
{
    /* fprintf(stderr, "SESSION QUEUE VAL CHANGE!\n"); */
    pthread_mutex_lock(&session->queued_ops.queued_val_changes_lock);
    enum jdaw_thread thread = endpoint_get_owner(ep);
    struct queued_val_change qvc = {
	ep,
	new_val,
	run_gui_cb
    };
    int num_queued = session->queued_ops.num_queued_val_changes[thread];
    bool overwritten = false;
    for (int i=0; i<num_queued; i++) {
	struct queued_val_change *qvc_check = session->queued_ops.queued_val_changes[thread] + i;
	if (qvc_check->ep == ep) {
	    /* fprintf(stderr, "already writing to endpoint! resetting value\n"); */
	    memcpy(qvc_check, &qvc, sizeof(struct queued_val_change));
	    overwritten = true;
	    break;
	}
    }
    if (!overwritten) {
	if (num_queued == MAX_QUEUED_OPS) {
	    pthread_mutex_unlock(&session->queued_ops.queued_val_changes_lock);
	    return 1;
	}
	/* fprintf(stderr, "adding new!\n"); */
	memcpy(session->queued_ops.queued_val_changes[thread] + num_queued, &qvc, sizeof(struct queued_val_change));
	session->queued_ops.num_queued_val_changes[thread]++;

    }
    /* struct queued_val_change *qvc = &session->queued_ops.queued_val_changes[thread][session->queued_ops.num_queued_val_changes[thread]]; */
    /* qvc->ep = ep; */
    /* qvc->new_val = new_val; */
    /* qvc->run_gui_cb = run_gui_cb; */
    /* for (int i=0; i<session->queued_ops.num_queued_val_changes[thread]; i++) { */
    /* 	if (memcmp(qvc, session->queued_ops.queued_val_changes[thread] + i, sizeof(struct queued_val_change)) == 0) { */
    /* 	    memcpy(session->queued_ops.queued_val_changes[thread] */
    /* 	} */

    /* session->queued_ops.num_queued_val_changes[thread]++; */
    pthread_mutex_unlock(&session->queued_ops.queued_val_changes_lock);
    return 0;
}


static JDAW_THREAD_LOCAL int MAX_DEFERRED = 32;
static JDAW_THREAD_LOCAL int num_deferred = 0;
static JDAW_THREAD_LOCAL Endpoint *deferred_eps[32];
static JDAW_THREAD_LOCAL EndptCb deferred_cbs[32];
static JDAW_THREAD_LOCAL enum jdaw_thread deferred_threads[32];

void session_flush_val_changes(Session *session, enum jdaw_thread thread)
{
    Timeline *tl = ACTIVE_TL;
    int32_t tl_now = timeline_get_play_pos_now(tl);
    pthread_mutex_lock(&session->queued_ops.queued_val_changes_lock);
    /* fprintf(stderr, "Flush %d val changes on thread %s\n", session->num_queued_val_changes[thread], thread==JDAW_THREAD_DSP ? "DSP" : "Main"); */
    for (int i=0; i<session->queued_ops.num_queued_val_changes[thread]; i++) {
	struct queued_val_change *qvc = &session->queued_ops.queued_val_changes[thread][i];
	Endpoint *ep = qvc->ep;
	/* Protected write */
	fprintf(stderr, "\tVal change flush \"%s\", intval %d\n", ep->local_id, qvc->new_val.int_v);
	pthread_mutex_lock(&ep->val_lock);
	jdaw_val_set_ptr(ep->val, ep->val_type, qvc->new_val);
	pthread_mutex_unlock(&ep->val_lock);
	if (ep->automation && ep->automation->write) {
	    automation_endpoint_write(ep, qvc->new_val, tl_now);
	}
	if (thread != JDAW_THREAD_MAIN && qvc->run_gui_cb && ep->gui_callback) {
	    session_queue_callback(session, ep, ep->gui_callback, JDAW_THREAD_MAIN);
	}
    }
    session->queued_ops.num_queued_val_changes[thread] = 0;
    pthread_mutex_unlock(&session->queued_ops.queued_val_changes_lock);
    for (int i=0; i<num_deferred; i++) {
	session_queue_callback(session, deferred_eps[i], deferred_cbs[i], deferred_threads[i]);
    }
    num_deferred = 0;
}

int session_queue_callback(Session *session, Endpoint *ep, EndptCb cb, enum jdaw_thread thread)
{
    /* fprintf(stderr, "QUEUE from thread %s\n", get_thread_name()); */
    int ret;
    if ((ret = pthread_mutex_trylock(&session->queued_ops.queued_callback_lock)) != 0) {
	if (num_deferred == MAX_DEFERRED) return 3;
	deferred_eps[num_deferred] = ep;
	deferred_cbs[num_deferred] = cb;
	deferred_threads[num_deferred] = thread;
	num_deferred++;
	return 2;
	/* return 2; */
    }
    int num_queued = session->queued_ops.num_queued_callbacks[thread];
    /* bool already_queued = false; */
    for (int i=0; i<num_queued; i++) {
	if (session->queued_ops.queued_callbacks[thread][i] == cb
	    && session->queued_ops.queued_callback_args[thread][i] == ep) {
	    fprintf(stderr, "Already queued...\n");
	    /* already_queued = true; */
	    goto unlock_and_exit;
	}
    }
    if (session->queued_ops.num_queued_callbacks[thread] == MAX_QUEUED_OPS) {
	pthread_mutex_unlock(&session->queued_ops.queued_callback_lock);
	return 1;
    }
    session->queued_ops.queued_callbacks[thread][session->queued_ops.num_queued_callbacks[thread]] = cb;
    session->queued_ops.queued_callback_args[thread][session->queued_ops.num_queued_callbacks[thread]] = ep;
    session->queued_ops.num_queued_callbacks[thread]++;
unlock_and_exit:
    if ((ret = pthread_mutex_unlock(&session->queued_ops.queued_callback_lock)) != 0) {
	fprintf(stderr, "Error in session_queue_callback unlock: %s\n", strerror(ret));
    }
    /* fprintf(stderr, "\t->completing queue on thread %s\n", get_thread_name()); */
    return 0;
}

void session_flush_callbacks(Session *session, enum jdaw_thread thread)
{
    int ret;
    if ((ret=pthread_mutex_lock(&session->queued_ops.queued_callback_lock)) != 0) {
	fprintf(stderr, "Error in session_flush_callbacks lock: %s\n", strerror(ret));
    }
    EndptCb *cb_arr = session->queued_ops.queued_callbacks[thread];
    Endpoint **arg_arr = session->queued_ops.queued_callback_args[thread];
    uint8_t num = session->queued_ops.num_queued_callbacks[thread];
    for (int i=0; i<num; i++) {
	fprintf(stderr, "\tCB flush \"%s\"\n", arg_arr[i]->local_id);
	cb_arr[i](arg_arr[i]);
    }
    session->queued_ops.num_queued_callbacks[thread] = 0;
    pthread_mutex_unlock(&session->queued_ops.queued_callback_lock);
}

int session_add_ongoing_change(Session *session, Endpoint *ep, enum jdaw_thread thread)
{
    
    pthread_mutex_lock(&session->queued_ops.ongoing_changes_lock);
    uint8_t *current_num = session->queued_ops.num_ongoing_changes + thread;
    if (*current_num == MAX_QUEUED_OPS) {
	pthread_mutex_unlock(&session->queued_ops.ongoing_changes_lock);
	return 1;
    }
    session->queued_ops.ongoing_changes[thread][*current_num] = ep;
    (*current_num)++;
    pthread_mutex_unlock(&session->queued_ops.ongoing_changes_lock);
    return 0;
}
void session_do_ongoing_changes(Session *session, enum jdaw_thread thread)
{
    pthread_mutex_lock(&session->queued_ops.ongoing_changes_lock);
    for (int i=0; i<session->queued_ops.num_ongoing_changes[thread]; i++) {
	Endpoint *ep = session->queued_ops.ongoing_changes[thread][i];
	if (ep->do_auto_incr) {
	    endpoint_continuous_change_do_incr(ep);
	}
    }
    pthread_mutex_unlock(&session->queued_ops.ongoing_changes_lock);
}

void session_flush_ongoing_changes(Session *session, enum jdaw_thread thread)
{
    pthread_mutex_lock(&session->queued_ops.ongoing_changes_lock);
    uint8_t *current_num = session->queued_ops.num_ongoing_changes + thread;
    for (int i=0; i<*current_num; i++) {
	Endpoint *ep = session->queued_ops.ongoing_changes[thread][i];
	endpoint_stop_continuous_change(ep);
    }
    *current_num = 0;
    pthread_mutex_unlock(&session->queued_ops.ongoing_changes_lock);
}
