/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    project_endpoint_ops.h

    * project-level functions managing operations related to the Endpoints API
 *****************************************************************************************************************/


#include "project.h"

int project_queue_val_change(Project *proj, Endpoint *ep, Value new_val, bool run_gui_cb);
void project_flush_val_changes(Project *proj, enum jdaw_thread thread);

int project_queue_callback(Project *proj, Endpoint *ep, EndptCb cb, enum jdaw_thread thread);
void project_flush_callbacks(Project *proj, enum jdaw_thread thread);

int project_add_ongoing_change(Project *proj, Endpoint *ep, enum jdaw_thread thread);
void project_do_ongoing_changes(Project *proj, enum jdaw_thread thread);
void project_flush_ongoing_changes(Project *proj, enum jdaw_thread thread);
