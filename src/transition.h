/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#ifndef JDAW_TRANSITION_H
#define JDAW_TRANSITION_H

#include "project.h"

typedef struct clip_boundary {
    Clip *clip;
    bool left;
} ClipBoundary;

void add_transition_from_tl(void);
void add_clip_transition(Clip *clip, bool left);

#endif