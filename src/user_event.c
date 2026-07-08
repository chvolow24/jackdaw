/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    user_event.c

    * record history of user actions
    * implement undo/redo
 *****************************************************************************************************************/

#include <stdlib.h>
#include "log.h"
#include "project.h"
#include "session.h"
#include "user_event.h"

extern Project *proj;

static void rectify_all_changes_saved_flag(UserEventHistory *history)
{
    if (history->next_undo) {        
        if (history->save_checkpoint_type == USER_EVENT_NEXT_UNDO
            /* CASE 1A: next undo point is same as at last save */
            && history->save_checkpoint_id == history->next_undo->id) {
            history->all_changes_saved = true;
        } else {
            /* CASE 1B: next undo exists but id not same */
            history->all_changes_saved = false;
        }
    } else if (history->oldest
        && history->save_checkpoint_type == USER_EVENT_OLDEST
        && history->save_checkpoint_id == history->oldest->id) {
        /* CASE 2: history all the way back, and oldest event is the same as at last save */
        history->all_changes_saved = true;
    } else if (history->oldest && history->oldest->id == 1 && history->save_checkpoint_id == -1) {
        /* CASE 3: history all the way back, and oldest event is first event; no save checkpoint yet (but events logged) */
        history->all_changes_saved = true;
    } else if (history->save_checkpoint_id == 0) {
        /* CASE 4: nothing has been done (so no unsaved changes) */
        history->all_changes_saved = true;
        return;
    } else {
        history->all_changes_saved = false;
    }
}

/* Returns 0 if action completed; 1 if no action available */
int user_event_do_undo(UserEventHistory *history)
{
    UserEvent *e = history->next_undo;
    if (!e) return 1;

    e->undo(e,
	    e->obj1,
	    e->obj2,
	    e->undo_val1,
	    e->undo_val2,
	    e->type1,
	    e->type2);
    if (e->previous) {
	history->next_undo = e->previous;
    } else {
	history->next_undo = NULL;
    }
    rectify_all_changes_saved_flag(history);
    return 0;   
}

/* Only do undo if undo fn is in known list */
void user_event_do_undo_selective(EventFn options[], int num_options)
{
    Session *session = session_get();
    UserEvent *e = session->history.next_undo;
    if (!e) return;
    fprintf(stderr, "undo selective!\n");
    for (int i=0; i<num_options; i++) {
	if (e->undo == options[i]) {
	    user_event_do_undo(&session->history);
	}
    }
}

/* Returns 0 if action completed; 1 if no action available */
int user_event_do_redo(UserEventHistory *history)
{
    UserEvent *e = NULL;
    if (!history->next_undo) {
	e = history->oldest;
	if (!e) return 1;
    } else if (!history->next_undo->next) return 1;
    if (!e) e = history->next_undo->next;
    if (!e) return 1;

    if (e->redo) {
	e->redo(e,
		e->obj1,
		e->obj2,
		e->redo_val1,
		e->redo_val2,
		e->type1,
		e->type2);
    }
    history->next_undo = e;
    rectify_all_changes_saved_flag(history);
    return 0;
}

static void user_event_destroy(UserEvent *e)
{
    /* Objs should be destroyed in custom undo or dispose functions,
       as needed */
    if (e->free_obj1) free(e->obj1);
    if (e->free_obj2) free(e->obj2);
    free(e);
}

/* Call when closing down session OR opening a new project */
void user_event_history_clear(UserEventHistory *history)
{
    UserEvent *test = history->oldest;
    UserEvent *delete;
    bool undoable = history->next_undo != NULL;
    bool redoable = false;
    while (test) {
	delete = test;
	test = test->next;
	if (undoable && delete->dispose) {
	    delete->dispose(delete, delete->obj1, delete->obj2, delete->undo_val1, delete->undo_val2, delete->type1, delete->type2);
	}
	if (redoable && delete->dispose_forward && delete->dispose_forward != delete->dispose) {
	    delete->dispose_forward(delete, delete->obj1, delete->obj2, delete->undo_val1, delete->undo_val2, delete->type1, delete->type2);
	}
	if (delete == history->next_undo) {
	    undoable = false;
	    redoable = true;
	}
	user_event_destroy(delete);
    }
    history->oldest = NULL;
    history->next_undo = NULL;
    history->len = 0;
}

UserEvent *user_event_push(
    EventFn undo_fn,
    EventFn redo_fn,
    EventFn dispose_fn,
    EventFn dispose_forward_fn,
    void *obj1,
    void *obj2,
    Value undo_val1,
    Value undo_val2,
    Value redo_val1,
    Value redo_val2,
    ValType type1,
    ValType type2,
    bool free_obj1,
    bool free_obj2
    )
{
    Session *session = session_get();
    if (!session) return NULL;
    if (!session->proj_initialized) return NULL;
    UserEventHistory *history = &session->history;
    if (history->pause) return NULL;
    if (history->current_macro) {
	history = history->current_macro;
    }
    UserEvent *e = calloc(1, sizeof(UserEvent));
    static uint64_t running_id = 1;
    e->id = running_id++;
    e->undo = undo_fn;
    e->redo = redo_fn;
    e->dispose = dispose_fn;
    e->dispose_forward = dispose_forward_fn;
    e->obj1 = obj1;
    e->obj2 = obj2;
    e->undo_val1 = undo_val1;
    e->undo_val2 = undo_val2;
    e->redo_val1 = redo_val1;
    e->redo_val2 = redo_val2;
    e->type1 = type1;
    e->type2 = type2;
    e->free_obj1 = free_obj1;
    e->free_obj2 = free_obj2;
    
    /* First case: history initialized, but we're all the way back */
    if (history->oldest && !history->next_undo) {
	UserEvent *newest = history->oldest;
	while (newest->next) {
	    newest = newest->next;
	}
	UserEvent *iter = newest;
	while (iter) {
	    /* next = iter->next; */
	    if (iter->dispose_forward) {
		log_tmp(LOG_DEBUG, "user event dispose fwd\n");
		iter->dispose_forward(
		    iter,
		    iter->obj1,
		    iter->obj2,
		    iter->redo_val1,
		    iter->redo_val2,
		    iter->type1, iter->type2);
	    }
	    UserEvent *to_destroy = iter;
	    iter = iter->previous;
	    user_event_destroy(to_destroy);
	    history->len--;
	}
	history->oldest = e;
	
    /* Second case: history is not initialized */
    } else if (!history->oldest) {
	history->oldest = e;
    /* Third case: history is initialized, but we're not at the front */
    } else if (history->next_undo->next) {
	/* NOTE:
	   When "disposing forward", events must be disposed in the same
	   order in which they were *undone* (not the order in which they were
	   originally done.
	*/

	UserEvent *next_redo = history->next_undo->next;

	/* First get last event in the chain */
	UserEvent *newest = next_redo;
	while (newest->next) {
	    newest = newest->next;
	}
	UserEvent *iter = newest;
	while (iter != history->next_undo) {
	    if (iter->dispose_forward) {
		/* log_tmp( */
		log_tmp(LOG_DEBUG, "user_event dispose fwd\n");
		iter->dispose_forward(
		    iter,
		    iter->obj1,
		    iter->obj2,
		    iter->redo_val1,
		    iter->redo_val2,
		    iter->type1, iter->type2);
		    
	    }
	    UserEvent *to_destroy = iter;
	    iter = iter->previous;
	    user_event_destroy(to_destroy);
	    history->len--;
	    /* if (iter == history->next_undo) { */
	    /* 	break; */
	    /* } */
	}
	history->next_undo->next = e;
	e->previous = history->next_undo;
    /* Fourth case: we're already at the end */
    } else {
	history->next_undo->next = e;
	e->previous = history->next_undo;
    }

    history->next_undo = e;
    
    e->index = history->len;
    history->len++;
    if (history->len > MAX_USER_EVENT_HISTORY_LEN) {
	UserEvent *old = history->oldest;
	if (old->dispose) {
	    log_tmp(LOG_DEBUG, "user event dispose\n");
	    old->dispose(
		e,
		old->obj1,
		old->obj2,
		old->undo_val1, /* not to be used */
		old->undo_val2, /* not to be used */
		old->type1, /* not to be used */
		old->type2 /* not to be used */
		);
	}
	history->oldest = old->next;
	history->oldest->previous = NULL;
	user_event_destroy(old);
	history->len--;
	old = history->oldest;
	while (old) {
	    old->index--;
	    old = old->next;
	}
    }
    /* At load time, checkpoint_id == 0 indicates there are no changes to save.
     As soon as a project state mutation occurs, this needs to be set to something different */
    if (history->save_checkpoint_id == 0) {
        history->save_checkpoint_id = -1;
    }
    rectify_all_changes_saved_flag(history);
    /* history->all_changes_saved = false; */
    return e;
}

void user_event_proj_save_checkpoint(UserEventHistory *history)
{
    history->all_changes_saved = true;
    if (history->next_undo) {
        history->save_checkpoint_id = history->next_undo->id;
        history->save_checkpoint_type = USER_EVENT_NEXT_UNDO;
    } else if (history->oldest) {
        history->save_checkpoint_id = history->oldest->id;
        history->save_checkpoint_type = USER_EVENT_OLDEST;
    } else {
        history->save_checkpoint_id = 0;
    }
}
bool user_event_proj_has_unsaved_changes(UserEventHistory *history)
{
    return !history->all_changes_saved;
}

void user_event_pause()
{
    Session *session = session_get();
    if (session)
	session->history.pause = true;
}

void user_event_unpause()
{
    Session *session = session_get();
    if (session)
	session->history.pause = false;
}

void user_event_undo_set_value(
    UserEvent *self,
    void *obj1,
    Value old_value,
    ValType type)
{
    switch (type) {
    case JDAW_FLOAT:
	*(float *)obj1 = old_value.float_v;
	break;
    case JDAW_DOUBLE:
	*(double *)obj1 = old_value.double_v;
	break;
    case JDAW_INT:
	*(int *)obj1 = old_value.int_v;
	break;
    case JDAW_UINT8:
	*(uint8_t *)obj1 = old_value.uint8_v;
	break;
    case JDAW_UINT16:
	*(uint16_t *)obj1 = old_value.uint16_v;
	break;
    case JDAW_UINT32:
	*(uint32_t *)obj1 = old_value.uint32_v;
	break;
    case JDAW_INT8:
	*(int8_t *)obj1 = old_value.int8_v;
	break;
    case JDAW_INT16:
	*(int16_t *)obj1 = old_value.int16_v;
	break;
    case JDAW_INT32:
	*(int32_t *)obj1 = old_value.int32_v;
	break;
    case JDAW_BOOL:
	*(bool *)obj1 = old_value.bool_v;
	break;
    case JDAW_DOUBLE_PAIR:
	*(double *)obj1 = old_value.double_pair_v[0];
	*((double *)obj1 + 1) = old_value.double_pair_v[1];
	break;
    case JDAW_PTR:
	*(void **)obj1 = old_value.ptr_v;
	break;
    }
}

void user_event_redo_set_value(
    UserEvent *self,
    void *obj1,
    Value new_value,
    ValType type)
{
    switch (type) {
    case JDAW_FLOAT:
	*(float *)obj1 = new_value.float_v;
	break;
    case JDAW_DOUBLE:
	*(double *)obj1 = new_value.double_v;
	break;
    case JDAW_INT:
	*(int *)obj1 = new_value.int_v;
	break;
    case JDAW_UINT8:
	*(uint8_t *)obj1 = new_value.uint8_v;
	break;
    case JDAW_UINT16:
	*(uint16_t *)obj1 = new_value.uint16_v;
	break;
    case JDAW_UINT32:
	*(uint32_t *)obj1 = new_value.uint32_v;
	break;
    case JDAW_INT8:
	*(int8_t *)obj1 = new_value.int8_v;
	break;
    case JDAW_INT16:
	*(int16_t *)obj1 = new_value.int16_v;
	break;
    case JDAW_INT32:
	*(int32_t *)obj1 = new_value.int32_v;
	break;
    case JDAW_BOOL:
	*(bool *)obj1 = new_value.bool_v;
	break;
    case JDAW_DOUBLE_PAIR:
	*(double *)obj1 = new_value.double_pair_v[0];
	*((double *)obj1 + 1) = new_value.double_pair_v[1];
	break;
    case JDAW_PTR:
	*(void **)obj1 = new_value.ptr_v;
    }
}


static void user_event_history_dispose(UserEventHistory *history)
{
    UserEvent *e = history->oldest;
    /* fprintf(stderr, "MACRO dispose\n"); */
    while (e) {
	/* fprintf(stderr, "\t->dispose sub event\n"); */
	if (e->dispose) {
	    e->dispose(e, e->obj1, e->obj2, e->undo_val1, e->undo_val2, e->type1, e->type2);
	}
	UserEvent *destroy = e;
	e = e->next;
	user_event_destroy(destroy);
    }
}

static void user_event_history_dispose_forward(UserEventHistory *history)
{
    UserEvent *e = history->oldest;
    /* fprintf(stderr, "MACRO dispose fwd\n"); */
    while (e) {
	/* fprintf(stderr, "\t->dispose fwd sub event\n"); */
	if (e->dispose_forward) {
	    e->dispose_forward(e, e->obj1, e->obj2, e->undo_val1, e->undo_val2, e->type1, e->type2);
	}
	UserEvent *destroy = e;
	e = e->next;
	user_event_destroy(destroy);	
    }
}

NEW_EVENT_FN(undo_user_event_macro, "undo (macro)")
{
    UserEventHistory *h = obj1;
    if (!h->always_sequence_order) {
	while (user_event_do_undo(h) == 0) {}
    } else {
	UserEvent *e = h->oldest;
	while (e) {
	    e->undo(
		e, e->obj1, e->obj2, e->undo_val1, e->undo_val2,
		e->type1, e->type2);
	    e = e->next;
	}
	h->next_undo = NULL;
    }
    char statstr_fmt[256];
    snprintf(statstr_fmt, 256, "(%d/%d) %s %s", session->history.len - self->index, session->history.len, "undo", h->macro_message); \
    status_set_undostr(statstr_fmt); \

    /* fprintf(stderr, "Done undo %s\n", h->macro_message); */
}}

NEW_EVENT_FN(redo_user_event_macro, "redo (macro)")
{
    UserEventHistory *h = obj1;
    while (user_event_do_redo(h) == 0) {}
    char statstr_fmt[256];
    snprintf(statstr_fmt, 256, "(%d/%d) %s %s", session->history.len - self->index, session->history.len, "redo", h->macro_message); \
    status_set_undostr(statstr_fmt); \

    /* snprintf(statstr_fmt, 255, "(%d/%d) %s", session->history.len - self->index, session->history.len, statstr); \ */
    /* status_set_undostr(statstr_fmt); \ */

    /* status_set_undostr(h->macro_message); */
    /* fprintf(stderr, "Done redo %s\n", h->macro_message); */

}}

NEW_EVENT_FN(dispose_user_event_macro, "")
{
    UserEventHistory *h = obj1;
    user_event_history_dispose(h);
    /* user_event_history_clear(h); */
    free(h);
}}

NEW_EVENT_FN(dispose_forward_user_event_macro, "")
{
    UserEventHistory *h = obj1;
    user_event_history_dispose_forward(h);
    /* user_event_history_clear(h); */
    free(h);
}}



void user_event_start_macro()
{
    Session *session = session_get();
    UserEventHistory *main = &session->history;
    if (main->current_macro) {
	log_tmp(LOG_ERROR, "user_event_start_macro: macro already in progress\n");
	return;
    }
    main->current_macro = calloc(1, sizeof(UserEventHistory));
}
void user_event_stop_macro(const char *message, bool always_sequence_order)
{
    Session *session = session_get();
    UserEventHistory *main = &session->history;
    UserEventHistory *macro = main->current_macro;
    if (!macro) {
	log_tmp(LOG_ERROR, "user_event_stop_macro: no macro in progress\n");
	return;
    }
    macro->macro_message = message;
    macro->always_sequence_order = always_sequence_order;
    main->current_macro = NULL;

    user_event_push(
	undo_user_event_macro,
	redo_user_event_macro,
	dispose_user_event_macro,
	dispose_forward_user_event_macro,
	macro, NULL,
	(Value){0}, (Value){0},
	(Value){0}, (Value){0},
	0, 0, false, false);
}
