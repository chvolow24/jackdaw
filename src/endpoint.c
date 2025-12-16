/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

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
#include "session_endpoint_ops.h"
#include "timeline.h"
#include "status.h"
#include "value.h"

typedef struct project Project;

int endpoint_init(
    Endpoint *ep,
    void *val,
    ValType t,
    const char *local_id,
    const char *display_name,
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
    ep->display_name = display_name;
    ep->owner_thread = owner_thread;
    ep->gui_callback = gui_cb;
    ep->proj_callback = proj_cb;
    ep->dsp_callback = dsp_cb;
    ep->xarg1 = xarg1;
    ep->xarg2 = xarg2;
    ep->xarg3 = xarg3;
    ep->xarg4 = xarg4;
    ep->block_undo = false;
    ep->automatable = true;
    jdaw_val_set_min(&ep->min, t);
    jdaw_val_set_max(&ep->max, t);

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

void endpoint_set_default_value(Endpoint *ep, Value default_val)
{
    ep->default_val = default_val;
}

void endpoint_set_label_fn(Endpoint *ep, LabelStrFn fn)
{
    ep->label_fn = fn;
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
/* fprintf(stderr, "undo %s %s\n", ep->local_id, ep->display_name); */
    endpoint_write(
	ep,
	val1,
	run_gui_cb,
	run_proj_cb,
	run_dsp_cb,
	false);

    char statstr_fmt[255];						
    snprintf(statstr_fmt, 255, "(%d/%d) undo/redo adj %s", session->history.len - self->index, session->history.len, ep->display_name);
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
    enum jdaw_thread owner = endpoint_get_owner(ep);
    ep->overwrite_val = endpoint_safe_read(ep, NULL);
    Session *session = session_get();
    int ret = 0;
    bool range_violation = false;
    if (ep->restrict_range) {
	if (jdaw_val_less_than(new_val, ep->min, ep->val_type)) {
	    new_val = ep->min;
	    range_violation = true;
	} else if (jdaw_val_less_than(ep->max, new_val, ep->val_type)) {
	    new_val = ep->max;
	    range_violation = true;
	}
    }
    if (range_violation) {
	const static int errstr_len = 32;
	char errstr[errstr_len];
	int index = 0;
	index += jdaw_val_to_str(errstr, errstr_len, ep->min, ep->val_type, 2);
	index += snprintf(errstr + index, errstr_len - index, " <= %s <= ", ep->local_id);
	index += jdaw_val_to_str(errstr + index, errstr_len - index, ep->max, ep->val_type, 2);
	status_set_errstr(errstr);
    }
    bool val_changed = !jdaw_val_equal(ep->last_write_val, new_val, ep->val_type);
    if (!val_changed && ep->write_has_occurred) return 0;
    if (!ep->write_has_occurred) {
	ep->last_write_val = endpoint_safe_read(ep, NULL);
    }
    ep->current_write_val = new_val;
    bool async_change_will_occur = false;
    /* Value change */
    ep->display_label = undoable || ep->changing; /* heuristic, ok */
    /* fprintf(stderr, "Owner? %d.. on owner? %d !session->playback.playing && owner == JDAW_THREAD_DSP %d?\n", owner, on_thread(owner), !session->playback.playing && owner == JDAW_THREAD_DSP); */
    if (on_thread(owner) || (!session->playback.playing && (owner == JDAW_THREAD_DSP || owner == JDAW_THREAD_PLAYBACK))) {
	pthread_mutex_lock(&ep->val_lock);
	jdaw_val_set_ptr(ep->val, ep->val_type, new_val);
	if (ep->automation && ep->automation->write) {
	    Timeline *tl = ACTIVE_TL;
	    int32_t tl_now = timeline_get_play_pos_now(tl);
	    automation_endpoint_write(ep, new_val, tl_now);
	}
	pthread_mutex_unlock(&ep->val_lock);
    } else {
	session_queue_val_change(session, ep, new_val, run_gui_cb);
	async_change_will_occur = true;
	ret = 1;
	/* } */
    }

    /* Callbacks */
    bool on_main = on_thread(JDAW_THREAD_MAIN);
    if (run_dsp_cb && ep->dsp_callback) {
	if (on_thread(JDAW_THREAD_DSP)) {
	    ep->dsp_callback(ep);
	} else {
	    if (session->playback.playing) {
		/* If ep owner assigned to playback thread, run DSP callbacks on that thread */
		enum jdaw_thread dst_thread = owner == JDAW_THREAD_PLAYBACK ? JDAW_THREAD_PLAYBACK : JDAW_THREAD_DSP;
		session_queue_callback(session, ep, ep->dsp_callback, dst_thread);
		async_change_will_occur = true;
	    } else if (session->midi_io.monitor_synth && owner == JDAW_THREAD_PLAYBACK) {
		session_queue_callback(session, ep, ep->dsp_callback, JDAW_THREAD_PLAYBACK);
		async_change_will_occur = true;
	    } else {
		ep->dsp_callback(ep);
	    }
	}	
    }
    if (run_proj_cb && ep->proj_callback) {
	if (on_main)
	    ep->proj_callback(ep);
	else {
	    session_queue_callback(session, ep, ep->proj_callback, JDAW_THREAD_MAIN);
	}
    }

    if (run_gui_cb && ep->gui_callback && !async_change_will_occur) {
	if (on_main) {
	    ep->gui_callback(ep);
	} else {
	    session_queue_callback(session, ep, ep->gui_callback, JDAW_THREAD_MAIN);
	}
    }
    
    /* if (run_gui_cb && ep->gui_callback) { */
    /* 	if (on_main) */
    /* 	    ep->gui_callback(ep); */
    /* 	else */
    /* 	    session_queue_callback(proj, ep, ep->gui_callback, JDAW_THREAD_MAIN); */
    /* } */
    
    /* Undo */
    if (undoable && !ep->block_undo) {
	if (!on_main) {
	    fprintf(stderr, "UH OH can't push event fn on thread that is not main\n");
	    return -1;
	}
	/* if (!jdaw_val_equal(old_val, new_val, ep->val_type)) { */
	    uint8_t callback_bitfield = 0;
	    /* if (run_gui_cb) callback_bitfield |= 0b001; */
	    /* if (run_proj_cb) callback_bitfield |= 0b010; */
	    /* if (run_dsp_cb) callback_bitfield |= 0b100; */
	    callback_bitfield = 0b111;
	    Value cb_matrix = {.uint8_v = callback_bitfield};
	    user_event_push(
		
		undo_redo_endpoint_write,
		undo_redo_endpoint_write,
		NULL, NULL,
		(void *)ep, NULL,
		ep->last_write_val, cb_matrix,
		new_val, cb_matrix,
		0, 0, false, false);
	/* } */
    }
    ep->last_write_val = new_val;
    ep->write_has_occurred = true;
    
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
    ep->cached_val = endpoint_safe_read(ep, NULL);
    /* ep->cached_owner = endpoint_get_owner(ep); */
    /* endpoint_set_owner(ep, thread); */

    endpoint_write(ep, new_value, true, true, true, false);

    ep->do_auto_incr = do_auto_incr;
    ep->incr = incr;

    Session *session = session_get();
    session_add_ongoing_change(session, ep, thread);
}

void endpoint_continuous_change_do_incr(Endpoint *ep)
{
    Value prev_val = jdaw_val_from_ptr(ep->val, ep->val_type);
    Value new_val = jdaw_val_add(prev_val, ep->incr, ep->val_type);
    endpoint_write(ep, new_val, true, true, true, false);
}

/* Called in session_endpoint_ops.c : session_flush_ongoing_changes() */
void endpoint_stop_continuous_change(Endpoint *ep)
{
    /* endpoint_set_owner(ep, ep->cached_owner); */
    uint8_t callback_bitfield = 0b111;
    /* callback_bitfield |= 0b001; */
    /* if (run_proj_cb) callback_bitfield |= 0b010; */
    /* if (run_dsp_cb) callback_bitfield |= 0b100; */
    Value cb_matrix = {.uint8_v = callback_bitfield};
    Value current_val = jdaw_val_from_ptr(ep->val, ep->val_type);
    if (!jdaw_val_equal(current_val, ep->cached_val, ep->val_type)) {
	user_event_push(
	    
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
    a->endpoint = ep;
}

void api_node_set_owner(APINode *node, enum jdaw_thread thread)
{
    for (int i=0; i<node->num_endpoints; i++) {
	endpoint_set_owner(node->endpoints[i], thread);
    }
    for (int i=0; i<node->num_children; i++) {
	api_node_set_owner(node->children[i], thread);
    }
}
