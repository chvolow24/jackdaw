/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    status.h

    * write text to the project status bar
 *****************************************************************************************************************/

#ifndef JDAW_STATUS_H

#define JDAW_STATUS_H
#define MAX_STATUS_STRLEN 255

void status_frame();
void status_set_statstr(const char *statstr);
void status_set_errstr(const char *errstr);
void status_set_undostr(const char *undostr);
void status_set_callstr(const char *callstr);
void status_cat_callstr(const char *catstr);
void status_stat_playspeed();
void status_stat_drag();

#endif
