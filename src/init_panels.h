/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    init_panels.h

    * horrible, procedural construction of "panels" (at top of GUI)
 *****************************************************************************************************************/


#ifndef JDAW_INIT_PANELS_H
#define JDAW_INIT_PANELS_H

typedef struct session Session;
typedef struct layout Layout;

void session_init_panels(Session *session);
void session_deinit_panels(Session *session);

#endif
