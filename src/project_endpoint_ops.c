/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    project_endpoint_ops.c

    * project-level functions managing operations related to the Endpoints API
 *****************************************************************************************************************/


#include "project_endpoint_ops.h"
#include "endpoint.h"
#include "timeline.h"

/* CALLBACKS */

/* Returns 0 on success, 1 if maximum number of callbacks reached */
int project_queue_val_change(Project *proj, Endpoint *ep, Value new_val, bool run_gui_cb)
{
    pthread_mutex_lock(&proj->queued_val_changes_lock);
    enum jdaw_thread thread = endpoint_get_owner(ep);
    if (proj->num_queued_val_changes[thread] == MAX_QUEUED_OPS) {
	pthread_mutex_unlock(&proj->queued_val_changes_lock);
	return 1;
    }
    struct queued_val_change *qvc = &proj->queued_val_changes[thread][proj->num_queued_val_changes[thread]];
    qvc->ep = ep;
    qvc->new_val = new_val;
    qvc->run_gui_cb = run_gui_cb;
    proj->num_queued_val_changes[thread]++;
    pthread_mutex_unlock(&proj->queued_val_changes_lock);
    return 0;
}


static JDAW_THREAD_LOCAL int MAX_DEFERRED = 32;
static JDAW_THREAD_LOCAL int num_deferred = 0;
static JDAW_THREAD_LOCAL Endpoint *deferred_eps[32];
static JDAW_THREAD_LOCAL EndptCb deferred_cbs[32];
static JDAW_THREAD_LOCAL enum jdaw_thread deferred_threads[32];

void project_flush_val_changes(Project *proj, enum jdaw_thread thread)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    int32_t tl_now = timeline_get_play_pos_now(tl);
    pthread_mutex_lock(&proj->queued_val_changes_lock);
    for (int i=0; i<proj->num_queued_val_changes[thread]; i++) {
	struct queued_val_change *qvc = &proj->queued_val_changes[thread][i];
	Endpoint *ep = qvc->ep;
	/* Protected write */
	pthread_mutex_lock(&ep->val_lock);
	jdaw_val_set_ptr(ep->val, ep->val_type, qvc->new_val);
	pthread_mutex_unlock(&ep->val_lock);
	if (ep->automation && ep->automation->write) {
	    automation_endpoint_write(ep, qvc->new_val, tl_now);
	}
	if (thread != JDAW_THREAD_MAIN && qvc->run_gui_cb && ep->gui_callback) {
	    project_queue_callback(proj, ep, ep->gui_callback, JDAW_THREAD_MAIN);
	}
    }
    proj->num_queued_val_changes[thread] = 0;
    pthread_mutex_unlock(&proj->queued_val_changes_lock);
    for (int i=0; i<num_deferred; i++) {
	project_queue_callback(proj, deferred_eps[i], deferred_cbs[i], deferred_threads[i]);
    }
    num_deferred = 0;
}

int project_queue_callback(Project *proj, Endpoint *ep, EndptCb cb, enum jdaw_thread thread)
{
    /* fprintf(stderr, "QUEUE from thread %s\n", get_thread_name()); */
    int ret;
    if ((ret = pthread_mutex_trylock(&proj->queued_callback_lock)) != 0) {
	if (num_deferred == MAX_DEFERRED) return 3;
	deferred_eps[num_deferred] = ep;
	deferred_cbs[num_deferred] = cb;
	deferred_threads[num_deferred] = thread;
	num_deferred++;
	return 2;
	/* return 2; */
    }
    if (proj->num_queued_callbacks[thread] == MAX_QUEUED_OPS) {
	pthread_mutex_unlock(&proj->queued_callback_lock);
	return 1;
    }
    proj->queued_callbacks[thread][proj->num_queued_callbacks[thread]] = cb;
    proj->queued_callback_args[thread][proj->num_queued_callbacks[thread]] = ep;
    proj->num_queued_callbacks[thread]++;
    if ((ret = pthread_mutex_unlock(&proj->queued_callback_lock)) != 0) {
	fprintf(stderr, "Error in project_queue_callback unlock: %s\n", strerror(ret));
    }
    /* fprintf(stderr, "\t->completing queue on thread %s\n", get_thread_name()); */
    return 0;
}

void project_flush_callbacks(Project *proj, enum jdaw_thread thread)
{
    int ret;
    if ((ret=pthread_mutex_lock(&proj->queued_callback_lock)) != 0) {
	fprintf(stderr, "Error in project_flush_callbacks lock: %s\n", strerror(ret));
    }
    EndptCb *cb_arr = proj->queued_callbacks[thread];
    Endpoint **arg_arr = proj->queued_callback_args[thread];
    uint8_t num = proj->num_queued_callbacks[thread];
    for (int i=0; i<num; i++) {
	cb_arr[i](arg_arr[i]);
    }
    proj->num_queued_callbacks[thread] = 0;
    pthread_mutex_unlock(&proj->queued_callback_lock);
}

int project_add_ongoing_change(Project *proj, Endpoint *ep, enum jdaw_thread thread)
{
    
    pthread_mutex_lock(&proj->ongoing_changes_lock);
    uint8_t *current_num = proj->num_ongoing_changes + thread;
    if (*current_num == MAX_QUEUED_OPS) {
	pthread_mutex_unlock(&proj->ongoing_changes_lock);
	return 1;
    }
    proj->ongoing_changes[thread][*current_num] = ep;
    (*current_num)++;
    pthread_mutex_unlock(&proj->ongoing_changes_lock);
    return 0;
}
void project_do_ongoing_changes(Project *proj, enum jdaw_thread thread)
{
    pthread_mutex_lock(&proj->ongoing_changes_lock);
    for (int i=0; i<proj->num_ongoing_changes[thread]; i++) {
	Endpoint *ep = proj->ongoing_changes[thread][i];
	if (ep->do_auto_incr) {
	    endpoint_continuous_change_do_incr(ep);
	}
    }
    pthread_mutex_unlock(&proj->ongoing_changes_lock);
}

void project_flush_ongoing_changes(Project *proj, enum jdaw_thread thread)
{
    pthread_mutex_lock(&proj->ongoing_changes_lock);
    uint8_t *current_num = proj->num_ongoing_changes + thread;
    for (int i=0; i<*current_num; i++) {
	Endpoint *ep = proj->ongoing_changes[thread][i];
	endpoint_stop_continuous_change(ep);
    }
    *current_num = 0;
    pthread_mutex_unlock(&proj->ongoing_changes_lock);
}
