/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
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
    endpoint.c

    * define API by which project paramters can be accessed and modified
    * groundwork for UDP API
    * see endpoint.h for more info
 *****************************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "endpoint.h"
#include "project_endpoint_ops.h"
#include "timeline.h"
#include "status.h"
#include "value.h"

typedef struct project Project;
extern Project *proj;

/* int project_queue_callback(Project *proj, Endpoint *ep, EndptCb cb, enum jdaw_thread thread); */
/* int project_queue_val_change(Project *proj, Endpoint *ep, Value new_val); */

int endpoint_init(
    Endpoint *ep,
    void *val,
    ValType t,
    const char *local_id,
    const char *undo_str,
    enum jdaw_thread owner_thread,
    EndptCb gui_cb,
    EndptCb proj_cb,
    EndptCb dsp_cb,
    void *xarg1, void *xarg2,
    void *xarg3, void *xarg4)
{
    ep->val = val;
    ep->val_type = t;
    ep->local_id = local_id;
    ep->undo_str = undo_str;
    ep->owner_thread = owner_thread;
    ep->gui_callback = gui_cb;
    ep->proj_callback = proj_cb;
    ep->dsp_callback = dsp_cb;
    ep->xarg1 = xarg1;
    ep->xarg2 = xarg2;
    ep->xarg3 = xarg3;
    ep->xarg4 = xarg4;

    int err;
    if ((err = pthread_mutex_init(&ep->val_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing ep val mutex: %s\n", strerror(err));
	exit(1);
    }
    if ((err = pthread_mutex_init(&ep->owner_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing ep owner mutex: %s\n", strerror(err));
	exit(1);
    }
    return 0;
}

void endpoint_set_allowed_range(Endpoint *ep, Value min, Value max)
{
    ep->min = min;
    ep->max = max;
    ep->restrict_range = true;
}

/* int enpoint_add_callback(Endpoint *ep, EndptCb fn, enum jdaw_thread thread) */
/* { */
/*     if (ep->num_callbacks == MAX_ENDPOINT_CALLBACKS) { */
/* 	return 1; */
/*     } */
/*     struct endpt_cb *cb = ep->callbacks + ep->num_callbacks; */
/*     cb->fn = fn; */
/*     cb->thread = thread; */
/*     return 0; */
/* } */

NEW_EVENT_FN(undo_redo_endpoint_write, "")
    Endpoint *ep = (Endpoint *)obj1;
    uint8_t cb_bf = val2.uint8_v;
    bool run_gui_cb = cb_bf & 0b001;
    bool run_proj_cb = cb_bf & 0b010;
    bool run_dsp_cb = cb_bf & 0b100;
    endpoint_write(
	ep,
	val1,
	run_gui_cb,
	run_proj_cb,
	run_dsp_cb,
	false);

    char statstr_fmt[255];						
    snprintf(statstr_fmt, 255, "(%d/%d) %s", proj->history.len - self->index, proj->history.len, ep->undo_str);
    status_set_undostr(statstr_fmt);
}

/* Return value is one of:
   0: value written synchronously
   1: value change scheduled other thread
   -1: ERROR: undo action can't be pushed on current thread
*/
int endpoint_write(
    Endpoint *ep,
    Value new_val,
    bool run_gui_cb,
    bool run_proj_cb,
    bool run_dsp_cb,
    bool undoable)
{
    int ret = 0;
    if (ep->restrict_range) {
	if (jdaw_val_less_than(new_val, ep->min, ep->val_type)) {
	    new_val = ep->min;
	} else if (jdaw_val_less_than(ep->max, new_val, ep->val_type)) {
	    new_val = ep->max;
	}
    }
    Value old_val;
    if (undoable) {
	old_val = endpoint_safe_read(ep, NULL);
    }
    bool async_change_will_occur = false;
    /* Value change */
    enum jdaw_thread owner = endpoint_get_owner(ep);
    if (on_thread(owner) || (!proj->playing && owner == JDAW_THREAD_DSP)) {
	pthread_mutex_lock(&ep->val_lock);
	jdaw_val_set_ptr(ep->val, ep->val_type, new_val);
	if (ep->automation && ep->automation->write) {
	    Timeline *tl = proj->timelines[proj->active_tl_index];
	    int32_t tl_now = timeline_get_play_pos_now(tl);
	    automation_endpoint_write(ep, new_val, tl_now);
	}
	pthread_mutex_unlock(&ep->val_lock);
    } else {
	/* if (!proj->playing && ep->owner_thread == JDAW_THREAD_DSP) { */
	/*     pthread_mutex_lock(&ep->lock); */
	/*     jdaw_val_set_ptr(ep->val, ep->val_type, new_val); */
	/*     pthread_mutex_unlock(&ep->lock);	     */
	/* } else { */
	project_queue_val_change(proj, ep, new_val, run_gui_cb);
	async_change_will_occur = true;
	ret = 1;
	/* } */
    }

    /* Callbacks */
    bool on_main = on_thread(JDAW_THREAD_MAIN);
    if (run_dsp_cb && ep->dsp_callback) {
	if (on_thread(JDAW_THREAD_DSP)) {
	    /* fprintf(stderr, "DSP CALLBACK SYNCHRONOUS\n"); */
	    ep->dsp_callback(ep);
	} else {
	    if (proj->playing) {
		/* fprintf(stderr, "DSP CALLBACK scheduled.....\n"); */
		project_queue_callback(proj, ep, ep->dsp_callback, JDAW_THREAD_DSP);
		async_change_will_occur = true;
	    } else {
		/* fprintf(stderr, "DSP CALLBACK SYNCHRONOUS\n"); */
		ep->dsp_callback(ep);
	    }
	}	
    }
    if (run_proj_cb && ep->proj_callback) {
	if (on_main)
	    ep->proj_callback(ep);
	else {
	    project_queue_callback(proj, ep, ep->proj_callback, JDAW_THREAD_MAIN);
	}
    }

    if (run_gui_cb && ep->gui_callback && !async_change_will_occur) {
	ep->gui_callback(ep);
    }
    
    /* if (run_gui_cb && ep->gui_callback) { */
    /* 	if (on_main) */
    /* 	    ep->gui_callback(ep); */
    /* 	else */
    /* 	    project_queue_callback(proj, ep, ep->gui_callback, JDAW_THREAD_MAIN); */
    /* } */
    
    /* Undo */
    if (undoable) {
	if (!on_main) {
	    fprintf(stderr, "UH OH can't push event fn on thread that is not main\n");
	    return -1;
	}
	if (!jdaw_val_equal(old_val, new_val, ep->val_type)) {
	    uint8_t callback_bitfield = 0;
	    if (run_gui_cb) callback_bitfield |= 0b001;
	    if (run_proj_cb) callback_bitfield |= 0b010;
	    if (run_dsp_cb) callback_bitfield |= 0b100;
	    Value cb_matrix = {.uint8_v = callback_bitfield};
	    user_event_push(
		&proj->history,
		undo_redo_endpoint_write,
		undo_redo_endpoint_write,
		NULL, NULL,
		(void *)ep, NULL,
		old_val, cb_matrix,
		new_val, cb_matrix,
		0, 0, false, false);
	}
    }
    
    return ret;
}

Value endpoint_unsafe_read(Endpoint *ep, ValType *vt)
{
    if (vt) {
	*vt = ep->val_type;
    }
    return jdaw_val_from_ptr(ep->val, ep->val_type);    
}

Value endpoint_safe_read(Endpoint *ep, ValType *vt)
{
    enum jdaw_thread owner = endpoint_get_owner(ep);
    if (on_thread(owner)) {
	/* fprintf(stderr, "DIRECT read\n"); */
	return endpoint_unsafe_read(ep, vt);
    } else {
	/* fprintf(stderr, "INDIRECT read\n"); */
	pthread_mutex_lock(&ep->val_lock);
	Value ret = endpoint_unsafe_read(ep, vt);
	pthread_mutex_unlock(&ep->val_lock);
	return ret;
    }   
}

/* PROBLEM: there may be queued value change operations */
void endpoint_set_owner(Endpoint *ep, enum jdaw_thread thread)
{
    pthread_mutex_lock(&ep->owner_lock);
    ep->owner_thread = thread;
    pthread_mutex_unlock(&ep->owner_lock);
}

enum jdaw_thread endpoint_get_owner(Endpoint *ep)
{
    enum jdaw_thread thread;
    pthread_mutex_lock(&ep->owner_lock);
    thread = ep->owner_thread;
    pthread_mutex_unlock(&ep->owner_lock);
    return thread;
}

void endpoint_start_continuous_change(
    Endpoint *ep,
    bool do_auto_incr,
    Value incr,
    enum jdaw_thread thread,
    Value new_value)
{
    if (ep->changing) return;
    ep->changing = true;
    ep->cached_owner = endpoint_get_owner(ep);
    endpoint_set_owner(ep, thread);

    endpoint_write(ep, new_value, true, true, true, true);

    ep->do_auto_incr = do_auto_incr;
    ep->incr = incr;
    ep->cached_val = endpoint_safe_read(ep, NULL);
    
    project_add_ongoing_change(proj, ep, thread);
}

void endpoint_continuous_change_do_incr(Endpoint *ep)
{
    Value prev_val = jdaw_val_from_ptr(ep->val, ep->val_type);
    Value new_val = jdaw_val_add(prev_val, ep->incr, ep->val_type);
    endpoint_write(ep, new_val, true, true, true, false);
}

void endpoint_stop_continuous_change(Endpoint *ep)
{
    endpoint_set_owner(ep, ep->cached_owner);
    uint8_t callback_bitfield = 0b111;
    /* callback_bitfield |= 0b001; */
    /* if (run_proj_cb) callback_bitfield |= 0b010; */
    /* if (run_dsp_cb) callback_bitfield |= 0b100; */
    Value cb_matrix = {.uint8_v = callback_bitfield};
    Value current_val = jdaw_val_from_ptr(ep->val, ep->val_type);
    if (!jdaw_val_equal(current_val, ep->cached_val, ep->val_type)) {
	user_event_push(
	    &proj->history,
	    undo_redo_endpoint_write,
	    undo_redo_endpoint_write,
	    NULL, NULL,
	    (void *)ep, NULL,
	    ep->cached_val, cb_matrix,
	    current_val, cb_matrix,
	    0, 0, false, false);
    }
    ep->changing = false;

}

void endpoint_bind_automation(Endpoint *ep, Automation *a)
{
    ep->automation = a;
}
