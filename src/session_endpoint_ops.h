/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    session_endpoint_ops.h

    * session-level functions managing operations related to the Endpoints API
    * see endpoints.h for motivation/context
 *****************************************************************************************************************/


#include "session.h"

int session_queue_val_change(Session *session, Endpoint *ep, Value new_val, bool run_gui_cb);
void session_flush_val_changes(Session *session, enum jdaw_thread thread);

int session_queue_callback(Session *session, Endpoint *ep, EndptCb cb, enum jdaw_thread thread);
void session_flush_callbacks(Session *session, enum jdaw_thread thread);

int session_add_ongoing_change(Session *session, Endpoint *ep, enum jdaw_thread thread);
void session_do_ongoing_changes(Session *session, enum jdaw_thread thread);
void session_flush_ongoing_changes(Session *session, enum jdaw_thread thread);
