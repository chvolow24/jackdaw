/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software iso
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/


/*****************************************************************************************************************
    project_endpoint_ops.c

    * project-level functions managing operations related to the Endpoints API
 *****************************************************************************************************************/


#include "project_endpoint_ops.h"
#include "endpoint.h"

/* CALLBACKS */

/* Returns 0 on success, 1 if maximum number of callbacks reached */
int project_queue_val_change(Project *proj, Endpoint *ep, Value new_val)
{
    pthread_mutex_lock(&proj->queued_val_changes_lock);
    enum jdaw_thread thread = ep->owner_thread;
    if (proj->num_queued_val_changes[thread] == MAX_QUEUED_OPS) {
	pthread_mutex_unlock(&proj->queued_val_changes_lock);
	return 1;
    }
    struct queued_val_change *qvc = &proj->queued_val_changes[thread][proj->num_queued_val_changes[thread]];
    qvc->ep = ep;
    qvc->new_val = new_val;
    proj->num_queued_val_changes[thread]++;
    pthread_mutex_unlock(&proj->queued_val_changes_lock);
    return 0;
}

void project_flush_val_changes(Project *proj, enum jdaw_thread thread)
{
    pthread_mutex_lock(&proj->queued_val_changes_lock);
    for (int i=0; i<proj->num_queued_val_changes[thread]; i++) {
	struct queued_val_change *qvc = &proj->queued_val_changes[thread][i];
	Endpoint *ep = qvc->ep;
	/* Protected write */
	pthread_mutex_lock(&ep->lock);
	jdaw_val_set_ptr(ep->val, ep->val_type, qvc->new_val);
	pthread_mutex_unlock(&ep->lock);
	/* fprintf(stderr, "\tFLUSHED %d/%d\n", i,proj->num_queued_val_changes[thread]); */
    }
    proj->num_queued_val_changes[thread] = 0;
    pthread_mutex_unlock(&proj->queued_val_changes_lock);
}

int project_queue_callback(Project *proj, Endpoint *ep, EndptCb cb, enum jdaw_thread thread)
{
    pthread_mutex_lock(&proj->queued_callback_lock);
    if (proj->num_queued_callbacks[thread] == MAX_QUEUED_OPS) {
	pthread_mutex_unlock(&proj->queued_callback_lock);
	return 1;
    }
    proj->queued_callbacks[thread][proj->num_queued_callbacks[thread]] = cb;
    proj->queued_callback_args[thread][proj->num_queued_callbacks[thread]] = ep;
    proj->num_queued_callbacks[thread]++;
    pthread_mutex_unlock(&proj->queued_callback_lock);
    return 0;
}

void project_flush_callbacks(Project *proj, enum jdaw_thread thread)
{
    pthread_mutex_lock(&proj->queued_callback_lock);
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
