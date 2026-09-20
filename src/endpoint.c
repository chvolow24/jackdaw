/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
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
#include "log.h"
#include "session_endpoint_ops.h"
#include "timeline.h"
#include "status.h"
#include "value.h"

typedef struct project Project;

int endpoint_init(
    Endpoint *ep,
    void *thread_local_val,
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
    shared_value_init(&ep->sv);
    ep->thread_local_val = thread_local_val;
    ep->val_type = t;
    ep->local_id = local_id;
    ep->display_name = display_name;
    ep->owner_thread = owner_thread;
    if (gui_cb) endpoint_register_callback(ep, JDAW_THREAD_MAIN, gui_cb);
    if (proj_cb) endpoint_register_callback(ep, JDAW_THREAD_MAIN, proj_cb);
    if (dsp_cb) endpoint_register_callback(ep, JDAW_THREAD_DSP, dsp_cb);
    /* ep->gui_callback = gui_cb; */
    /* ep->proj_callback = proj_cb; */
    /* ep->dsp_callback = dsp_cb; */
    ep->xarg1 = xarg1;
    ep->xarg2 = xarg2;
    ep->xarg3 = xarg3;
    ep->xarg4 = xarg4;
    ep->block_undo = false;
    ep->automatable = true;
    jdaw_val_set_min(&ep->min, t);
    jdaw_val_set_max(&ep->max, t);
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
    ep->has_default_val = true;
    ep->default_val = default_val;
}

/* Convenience function to be used at init time */
void endpoint_write_default(Endpoint *ep)
{
    if (!ep->has_default_val) {
	log_tmp(LOG_WARN, "Endpoint %s has no default value.\n", ep->local_id);
	return;
    }
    endpoint_write(ep, ep->default_val, true, true, true, false);    
}

void endpoint_set_label_fn(Endpoint *ep, LabelStrFn fn)
{
    ep->label_fn = fn;
}

void endpoint_register_callback(
    Endpoint *ep,
    enum jdaw_thread thread,
    EndptCb cb)
{
    MAIN_THREAD_ONLY(endpoint_register_callback);
    int num = atomic_load_explicit(&ep->num_registered_callbacks[thread], memory_order_relaxed);
    if (num >= MAX_ENDPOINT_CALLBACKS) {
        log_tmp(LOG_ERROR, "Max endpoint callbacks registered for %s\n", ep->local_id);
        return;
    }
    atomic_store_explicit(&ep->registered_callbacks[thread][num], cb, memory_order_relaxed);
    atomic_compare_exchange_strong_explicit(
        &ep->num_registered_callbacks[thread],
        &num,
        num + 1,
        memory_order_relaxed, memory_order_relaxed);
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

#define EP_WRITE_SAME_THREAD 0
#define EP_WRITE_OTHER_THREAD 1
#define EP_WRITE_NO_CHANGE 2
#define EP_WRITE_RANGE_VIOLATION_SAME_THREAD 10
#define EP_WRITE_RANGE_VIOLATION_OTHER_THREAD 11
#define EP_WRITE_ERROR_UNDO -1

#define EP_ERRSTR_LEN 32

static void set_thread_local_val_cb(Endpoint *ep)
{
    Value new_val = shared_value_read(&ep->sv);
    char dst[64];
    jdaw_val_to_str(dst, 64, new_val, ep->val_type, 2);
    jdaw_val_set_ptr(ep->thread_local_val, ep->val_type, new_val);    
}

/* Return value is one of:
   0: value written synchronously
   1: value change scheduled other thread
   2: no change
   +10: range violation
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
    /* fprintf(stderr, "OK Write endpoint %s, on thread %s, owner %s\n", ep->local_id, get_current_thread_name(), get_thread_name(owner)); */
    /* ep->overwrite_val = endpoint_read(ep, NULL); */
    Value old_val = endpoint_read(ep, NULL);
    Session *session = session_get();
    int ret = 0;
    bool range_violation = false;
    /* Value range_violation_write_val = new_val; */
    if (ep->restrict_range) {
	/* char ep_route[256]; */
	/* api_endpoint_get_route(ep, ep_route, 256); */
	/* fprintf(stderr, "Testing double range %f < %f < %f\t<=== %s\n", ep->min.double_v, new_val.double_v, ep->max.double_v, ep_route); */
	if (jdaw_val_less_than(new_val, ep->min, ep->val_type)) {
	    new_val = ep->min;
	    range_violation = true;
	} else if (jdaw_val_less_than(ep->max, new_val, ep->val_type)) {
	    new_val = ep->max;
	    range_violation = true;
	}
    }
    if (range_violation) {
	ret += 10;
	char errstr[EP_ERRSTR_LEN];
	int index = 0;
	index += jdaw_val_to_str(errstr, EP_ERRSTR_LEN, ep->min, ep->val_type, 2);
	index += snprintf(errstr + index, EP_ERRSTR_LEN - index, " <= %s <= ", ep->local_id);
	index += jdaw_val_to_str(errstr + index, EP_ERRSTR_LEN - index, ep->max, ep->val_type, 2);
	status_set_errstr(errstr);
    }
    shared_value_write(&ep->sv, new_val);
    bool val_changed = !jdaw_val_equal(old_val, new_val, ep->val_type);
    bool write_has_occurred = false;
    if (!val_changed &&
        (write_has_occurred = atomic_load_explicit(&ep->write_has_occurred, memory_order_relaxed))) {
        return EP_WRITE_NO_CHANGE;
    }
    
    ep->display_label = undoable || ep->changing; /* heuristic, ok */

    bool async_thread_loc_val_change = false;
    if (
	on_thread(owner)
	|| (on_thread(JDAW_THREAD_MAIN) && !thread_is_active(owner))) {
        
        /* Set thread local proxy for plan reads on owner thread */
	jdaw_val_set_ptr(ep->thread_local_val, ep->val_type, new_val);
	if (ep->automation && ep->automation->write) {
	    Timeline *tl = ACTIVE_TL;
	    int32_t tl_now = timeline_get_play_pos_now(tl);
	    automation_endpoint_write(ep, new_val, tl_now);
	}
    } else {
        async_thread_loc_val_change = true;
        struct queued_cb cb;
        cb.cb = set_thread_local_val_cb;
        cb.ep = ep;
        char dst[64];
        jdaw_val_to_str(dst, 64, new_val, ep->val_type, 2);
        session_enqueue_callback(owner, cb);
	/* session_queue_val_change(session, ep, new_val, run_gui_cb); */
	ret += EP_WRITE_OTHER_THREAD;
    }

    /* Callbacks v2 */
    for (enum jdaw_thread t=0; t<NUM_JDAW_THREADS; t++) {
        int num = atomic_load_explicit(&ep->num_registered_callbacks[t], memory_order_relaxed);
        for (int i=0; i<num; i++) {
            EndptCb cb = atomic_load_explicit(&ep->registered_callbacks[t][i], memory_order_relaxed);
            if (t == owner && async_thread_loc_val_change) {
                goto enqueue;
            } else if (on_thread(t)) {
                cb(ep);
            } else if (on_thread(JDAW_THREAD_MAIN) && !thread_is_active(t)) {
                /* Callbacks fall back to main if:
                   - current write is on main and
                   - destination thread is not active;
                */
                cb(ep);
            } else {
            enqueue:
                (void)0;
                struct queued_cb cbs = (struct queued_cb){cb, ep};
                session_enqueue_callback(t, cbs);
            }
        }
    }

    /* Callbacks */
    /* bool on_main = on_thread(JDAW_THREAD_MAIN); */
    /* if (run_dsp_cb && ep->dsp_callback) { */
    /*     if (on_thread(JDAW_THREAD_DSP)) { */
    /*         ep->dsp_callback(ep); */
    /*     } else { */
    /*         if (session->playback.playing || session->audio_io.playback_conn->playing) { */
    /*     	/\* If ep owner assigned to playback thread, run DSP callbacks on that thread *\/ */
    /*     	enum jdaw_thread dst_thread = owner == JDAW_THREAD_PLAYBACK ? JDAW_THREAD_PLAYBACK : JDAW_THREAD_DSP; */
    /*     	if (dst_thread == JDAW_THREAD_DSP && !session->playback.playing && session->audio_io.playback_conn->playing) { */
    /*     	    dst_thread = JDAW_THREAD_PLAYBACK; */
    /*     	} */
    /*     	int ret = session_queue_callback(session, ep, ep->dsp_callback, dst_thread); */
    /*     	if (ret == 3) { */
    /*     	    log_tmp(LOG_ERROR, "Error: call to queue callback for ep \"%s\" could not be deferred.\n", ep->local_id); */
    /*     	} */
    /*     	async_change_will_occur = true; */
    /*         /\* } else if (session->midi_io.monitor_synth && owner == JDAW_THREAD_PLAYBACK) { *\/ */
    /*         /\* 	session_queue_callback(session, ep, ep->dsp_callback, JDAW_THREAD_PLAYBACK); *\/ */
    /*         /\* 	async_change_will_occur = true; *\/ */
    /*         } else { */
    /*     	ep->dsp_callback(ep); */
    /*         } */
    /*     }	 */
    /* } */
    /* if (run_proj_cb && ep->proj_callback) { */
    /*     if (on_main) */
    /*         ep->proj_callback(ep); */
    /*     else { */
    /*         session_queue_callback(session, ep, ep->proj_callback, JDAW_THREAD_MAIN); */
    /*     } */
    /* } */

    /* if (run_gui_cb && ep->gui_callback && !async_change_will_occur) { */
    /*     if (on_main) { */
    /*         ep->gui_callback(ep); */
    /*     } else { */
    /*         session_queue_callback(session, ep, ep->gui_callback, JDAW_THREAD_MAIN); */
    /*     } */
    /* } */
    
    /* if (run_gui_cb && ep->gui_callback) { */
    /* 	if (on_main) */
    /* 	    ep->gui_callback(ep); */
    /* 	else */
    /* 	    session_queue_callback(proj, ep, ep->gui_callback, JDAW_THREAD_MAIN); */
    /* } */
    
    /* Undo */
    if (undoable && !ep->block_undo) {
	if (!on_thread(JDAW_THREAD_MAIN)) {
	    fprintf(stderr, "UH OH can't push event fn on thread that is not main\n");
	    return EP_WRITE_ERROR_UNDO;
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
	        old_val, cb_matrix,
		new_val, cb_matrix,
		0, 0, false, false);
	/* } */
    }
    /* ep->last_write_val = new_val; */
    atomic_store_explicit(&ep->write_has_occurred, true, memory_order_relaxed);
    
    return ret;
}

/* Value endpoint_unsafe_read(Endpoint *ep, ValType *vt) */
/* { */
/*     if (vt) { */
/* 	*vt = ep->val_type; */
/*     } */
/*     if (ep->thread_local_ */
/*     return jdaw_val_from_ptr(ep->val, ep->val_type);     */
/* } */

Value endpoint_read(Endpoint *ep, ValType *vt)
{
    if (vt) *vt = ep->val_type;
    /* return shared_value_read(&ep->sv); */
    enum jdaw_thread owner = endpoint_get_owner(ep);
    if (on_thread(owner) && ep->thread_local_val) {
        return jdaw_val_from_ptr(ep->thread_local_val, ep->val_type);
    } else {
        return shared_value_read(&ep->sv);
    }
}

/* PROBLEM: there may be queued value change operations */
void endpoint_set_owner(Endpoint *ep, enum jdaw_thread thread)
{
    atomic_store(&ep->owner_thread, thread);
}

enum jdaw_thread endpoint_get_owner(Endpoint *ep)
{
    return atomic_load(&ep->owner_thread);
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
    ep->cached_val = endpoint_read(ep, NULL);
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
    Value prev_val = endpoint_read(ep, NULL);//jdaw_val_from_ptr(ep->val, ep->val_type);
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
    Value current_val = endpoint_read(ep, NULL);//jdaw_val_from_ptr(ep->val, ep->val_type);
    if (!ep->block_undo && !jdaw_val_equal(current_val, ep->cached_val, ep->val_type)) {
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
