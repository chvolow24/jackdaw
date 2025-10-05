/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    project_draw.h

    * top-level draw function
 *****************************************************************************************************************/


#ifndef JDAW_PROJECT_DRAW_H
#define JDAW_PROJECT_DRAW_H

#include <stdbool.h>

void project_draw();

void draw_continuation_arrows(int x, int top_y, int h, bool point_left);

#endif
