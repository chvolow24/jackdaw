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
    user_event.h

    * define function prototypes for undo/redo class of functions
    * define user event struct, which is allocated when an undoable action occurs
    * define a UserEventHistory to be stored on global 'proj' obj
 *****************************************************************************************************************/


#ifndef JDAW_USER_EVENT_H
#define JDAW_USER_EVENT_H

#include "value.h"


#define MAX_USER_EVENT_HISTORY_LEN 50
#define EVENT_FN_DECL(name) void name(UserEvent *, void *, void *, Value, Value);
#define NEW_EVENT_FN(name, statstr)						\
    void name(UserEvent *self, void *obj1, void *obj2, Value val1, Value val2, ValType type1, ValType type2) { \
    if (strlen(statstr) > 0) { \
        char statstr_fmt[255];						\
        snprintf(statstr_fmt, 255, "(%d/%d) %s", proj->history.len - self->index, proj->history.len, statstr); \
        status_set_undostr(statstr_fmt); \
    } \

typedef struct user_event UserEvent;
typedef void (*EventFn)(
    UserEvent *self,
    void *obj1,
    void *obj2,
    Value val1,
    Value val2,
    ValType type1,
    ValType type2);


typedef struct user_event UserEvent;
typedef struct user_event_history UserEventHistory;

typedef struct user_event {
    EventFn undo;
    EventFn redo;
    EventFn dispose;
    EventFn dispose_forward;
    void *obj1;
    void *obj2;
    Value undo_val1;
    Value undo_val2;
    Value redo_val1;
    Value redo_val2;
    ValType type1;
    ValType type2;
    bool free_obj1;
    bool free_obj2;

    int index;
    UserEventHistory *history;
    UserEvent *next;
    UserEvent *previous;
} UserEvent;


typedef struct user_event_history {
    UserEvent *most_recent;
    UserEvent *oldest;
    UserEvent *next_undo;
    int len;
} UserEventHistory;


int user_event_do_undo(UserEventHistory *history);
int user_event_do_redo(UserEventHistory *history);

UserEvent *user_event_push(
    UserEventHistory *history,
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
    bool free_obj2);

void user_event_undo_set_value(
    UserEvent *self,
    void *obj1,
    Value old_value,
    ValType type);

void user_event_redo_set_value(
    UserEvent *self,
    void *obj1,
    Value new_value,
    ValType type);

void user_event_history_destroy(UserEventHistory *history);

#endif
