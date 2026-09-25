/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    session_endpoint_ops.h

    * session-level functions managing operations related to the Endpoints API
    * see endpoints.h for motivation/context
 *****************************************************************************************************************/


#include "session.h"

void session_add_ongoing_change(Endpoint *ep, enum jdaw_thread thread);
int session_do_ongoing_changes(enum jdaw_thread thread);
void session_clear_ongoing_changes(enum jdaw_thread thread);
void session_clear_all_ongoing_changes();

void session_enqueue_callback(enum jdaw_thread for_thread, struct queued_cb cb);
/* Return number of writes */
int session_run_thread_callbacks(enum jdaw_thread thread);
/* Use when closing project */
void session_clear_all_queued_callbacks();
