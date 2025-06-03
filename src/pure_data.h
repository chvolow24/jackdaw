/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    pure_data.h

    * somewhat experimental shared memory IPC with pure data
 *****************************************************************************************************************/


#include <semaphore.h>
#include "project.h"

int pd_jackdaw_shm_init();
void pd_signal_termination_of_jackdaw();
void *pd_jackdaw_record_on_thread(void *arg);
