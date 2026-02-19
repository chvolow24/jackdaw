/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    status.h

    * write messages to the project status bar (bottom of screen)
    * several distinct message types:
        - errors (errstr)
        - user function calls (callstr)
        - undo/redos (undostr)
        - playback speed changes
        - clip drag state (clipref.h)
*****************************************************************************************************************/

#ifndef JDAW_STATUS_H

#define JDAW_STATUS_H
#define MAX_STATUS_STRLEN 255

void status_frame();
void status_set_statstr(const char *fmt, ...);
/* Thread-safe */
void status_set_errstr(const char *fmt, ...);
void status_set_undostr(const char *undostr);
void status_set_callstr(const char *callstr);
void status_cat_callstr(const char *catstr);
void status_set_alertstr(const char *fmt, ...);
void status_set_sticky_alert_str(const char *fmt, ...);
void status_stat_playspeed();
void status_stat_drag();

#endif
