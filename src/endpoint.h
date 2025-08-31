/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    endpoint.h

    * define API by which project paramters can be accessed and modified
    * groundwork for UDP API
 *****************************************************************************************************************/

/*

  Jackdaw object parameters can be modified from within the program in normal C-like ways.
  E.g., it is legal to update the volume of a track object like this:
  
  Track *track = (...);
  track->vol = 0.0;
      
  However, it is sometimes helpful to place such modifications behind the API defined here,
  for the following reasons:

  1. THREAD SAFETY
      Endpoint write operations are legal from ANY thread
  2. UNDO/REDO
      Modifications to endpoint parameters that are undoable are handled by this API,
      so the event functions don't need to be separately implemented
  3. IPC:
      External applications can safely modify Jackdaw project parameters through this API


  CONCURRENCY MODEL

  Each value protected by an endpoint is assigned an "owner" thread. This thread should be
  one on which speedy read/write operations are most beneficial.

  Write ONLY occur on the owner thread, but can be initiated from any thread.

  * Writes:
      - on owner thread: occurs immediately, in mutex critical region
      - on other thread: queued for later execution on the owner thread.
  * Reads:
      - on owner thread: occurs immediately; NO LOCK
      - on other thread: occurs immediately, protected by mutex
 */


#ifndef JDAW_ENDPOINT_H
#define JDAW_ENDPOINT_H

#include "automation.h"
#include "thread_safety.h"
#include "value.h"

#define MAX_ENDPOINT_CALLBACKS 4

typedef struct endpoint Endpoint;
typedef void (*EndptCb)(Endpoint *);

/* /\* Callbacks are stored with their designated thread of execution *\/ */
/* struct endpt_cb { */
/*     EndptCb fn; */
/*     enum jdaw_thread thread; */
/* }; */

/* typedef struct jackdaw_api Jackdaw_API; */
typedef struct api_node APINode;
typedef struct api_hash_node APIHashNode;
typedef struct endpoint {
    
    void *val;
    ValType val_type;
    Value current_write_val; /* Set at start of write operation */
    Value last_write_val; /* Set at end of write operation */
    Value overwrite_val; /* Set at start of write operation */
    bool write_has_occurred;
    Value cached_val; /* For undo */
    bool restrict_range;
    Value min;
    Value max;
    Value default_val;
    /* struct endpt_cb callbacks[MAX_ENDPOINT_CALLBACKS]; */
    /* uint8_t num_callbacks; */
    
    EndptCb proj_callback; /* Main thread -- update project state outside target parameter */
    EndptCb gui_callback; /* Main thread -- update GUI state */
    EndptCb dsp_callback; /* DSP thread */
    
    pthread_mutex_t val_lock;
    pthread_mutex_t owner_lock;
    enum jdaw_thread owner_thread;
    enum jdaw_thread cached_owner;

    const char *local_id;
    const char *display_name;

    bool block_undo;
    /* const char *undo_str; */

    void *xarg1;
    void *xarg2;
    void *xarg3;
    void *xarg4;

    /* Continuous changes */
    bool do_auto_incr;
    Value incr;
    bool changing;

    /* Bindings */
    bool automatable;
    Automation *automation;
    LabelStrFn label_fn;

    /* API */
    APINode *parent;
    APIHashNode *hash_node;
} Endpoint;

int endpoint_init(
    Endpoint *ep,
    void *val,
    ValType t,
    const char *local_id,
    const char *display_name,
    /* const char *undo_str, */
    enum jdaw_thread owner_thread,
    EndptCb gui_cb,
    EndptCb proj_cb,
    EndptCb dsp_cb,
    void *xarg1, void *xarg2,
    void *xarg3, void *xarg4);

void endpoint_set_allowed_range(Endpoint *ep, Value min, Value max);
void endpoint_set_default_value(Endpoint *ep, Value default_val);
/* int endpoint_add_callback( */
/*     Endpoint *ep, */
/*     EndptCb fn, */
/*     enum jdaw_thread thread); */

/* Safely modify an endpoint's target value */
int endpoint_write(
    Endpoint *ep,
    Value new_val,
    bool run_gui_cb,
    bool run_proj_cb,
    bool run_dsp_cb,
    bool undoable);

/* int endpoint_read(Endpoint *ep, Value *dst_val, ValType *dst_vt); */
Value endpoint_unsafe_read(Endpoint *ep, ValType *vt);
Value endpoint_safe_read(Endpoint *ep, ValType *vt);

void endpoint_set_owner(Endpoint *ep, enum jdaw_thread thread);
enum jdaw_thread endpoint_get_owner(Endpoint *ep);

void endpoint_start_continuous_change(
    Endpoint *ep,
    bool do_auto_incr,
    Value incr,
    enum jdaw_thread thread,
    Value new_value);
void endpoint_continuous_change_do_incr(Endpoint *ep);
void endpoint_stop_continuous_change(Endpoint *ep);

/* int endpoint_register(Endpoint *ep, Jackdaw_API *api); */

void endpoint_bind_automation(Endpoint *ep, Automation *a);
void endpoint_set_label_fn(Endpoint *ep, LabelStrFn fn);
void api_node_set_owner(APINode *node, enum jdaw_thread thread);

#endif
