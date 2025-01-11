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
 *****************************************************************************************************************/

/*

  Jackdaw object parameters can be modified from within the program in normal C-like ways.
  E.g., it is legal to update the volume of a track object like this:
  
      Track *track = (...);
      track->vol = 0.0;
      
  However, it is sometimes helpful to place such modifications behind the API defined here, for the following reasons:

  1. THREAD SAFETY
      Endpoint write operations are legal from ANY thread
  2. UNDO/REDO
      Modifications to endpoint parameters that are undoable are handled by this API, so the event functions don't need to be separately implemented
  3. IPC:
      External applications can safely modify Jackdaw project parameters through this API
  
 */
#include <stdlib.h>
#include <string.h>
#include "endpoint.h"
#include "value.h"

typedef struct project Project;
extern Project *proj;

int project_queue_callback(Project *proj, Endpoint *ep, EndptCb cb, enum jdaw_thread thread);
int project_queue_val_change(Project *proj, void *target, ValType t, Value new_val, enum jdaw_thread thread);

int endpoint_init(
    Endpoint *ep,
    void *val,
    ValType t,
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
    /* if ((err = pthread_mutex_init(&ep->lock, NULL)) != 0) { */
    /* 	fprintf(stderr, "Error initializing mutex: %s\n", strerror(err)); */
    /* 	return 1; */
    /* } */
    return 0;
}

int endpoint_write(
    Endpoint *ep,
    Value new_val,
    bool run_gui_cb,
    bool run_proj_cb,
    bool run_dsp_cb)
{
    /* pthread_mutex_lock(&ep->lock); */
    if (on_thread(ep->owner_thread)) {
	fprintf(stderr, "SETTING DIRECT\n");
	jdaw_val_set_ptr(ep->val, ep->val_type, new_val);
    } else {
	fprintf(stderr, "QUEUEING\n");
	project_queue_val_change(proj, ep->val, ep->val_type, new_val, ep->owner_thread);
    }
    /* pthread_mutex_unlock(&ep->lock); */

    bool on_main = on_thread(JDAW_THREAD_MAIN);
    if (run_gui_cb) {
	if (on_main)
	    ep->gui_callback(ep);
	else
	    project_queue_callback(proj, ep, ep->gui_callback, JDAW_THREAD_MAIN);
    }
    if (run_proj_cb) {
	if (on_main)
	    ep->proj_callback(ep);
	else
	    project_queue_callback(proj, ep, ep->proj_callback, JDAW_THREAD_MAIN);
    }
    if (run_dsp_cb) {
	if (on_thread(JDAW_THREAD_DSP))
	    ep->dsp_callback(ep);
	else
	    project_queue_callback(proj, ep, ep->dsp_callback, JDAW_THREAD_DSP);
	
    }
    return 0;
}
