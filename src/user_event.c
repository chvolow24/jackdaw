/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
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
    user_event.c

    * record history of user actions
    * implement undo/redo
 *****************************************************************************************************************/

#include <stdlib.h>
#include "user_event.h"

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
    return 0;
    
}

/* Returns 0 if action completed; 1 if no action available */
int user_event_do_redo(UserEventHistory *history)
{
    UserEvent *e = NULL;
    if (!history->next_undo) {
	e = history->oldest;
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
    return 0;
}

static void user_event_destroy(UserEvent *e)
{
    /* Objs should be destroyed in custom undo or dispose functions,
       as needed */
    if (e->free_obj1) free(e->obj1);
    if (e->free_obj2) free(e->obj2);
}

UserEvent *user_event_push(
    UserEventHistory *history,
    EventFn undo_fn,
    EventFn redo_fn,
    EventFn dispose_fn,
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
    UserEvent *e = calloc(1, sizeof(UserEvent));
    e->undo = undo_fn;
    e->redo = redo_fn;
    e->dispose = dispose_fn;
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
	UserEvent *iter = history->oldest;
	UserEvent *next = iter->next;
	while (iter) {
	    next = iter->next;
	    user_event_destroy(iter);
	    iter = next;
	    history->len--;
	}
	history->oldest = e;
	history->most_recent = e;
	
    /* Second case: history is not initialized */
    } else if (!history->oldest) {
	history->oldest = e;
    /* Third case: history is initialized, but we're not at the front */
    } else if (history->next_undo != history->most_recent) {
	UserEvent *iter = history->next_undo->next;
	UserEvent *next = iter->next;
	while (iter) {
	    next = iter->next;
	    user_event_destroy(iter);
	    iter = next;
	    history->len--;
	}
	history->next_undo->next = e;
	e->previous = history->next_undo;
    /* Fourth case: we're already at the end */
    } else {
	history->next_undo->next = e;
	e->previous = history->next_undo;
    }

    history->next_undo = e;
    history->most_recent = e;
    
    e->index = history->len;
    history->len++;
    if (history->len > MAX_USER_EVENT_HISTORY_LEN) {
	UserEvent *old = history->oldest;
	if (old->dispose) {
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
    return e;
}
