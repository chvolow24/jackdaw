/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    animation.h

    * Typedefs related to "animations"
 *****************************************************************************************************************/

#ifndef JDAW_ANIMATION_H
#define JDAW_ANIMATION_H

typedef struct project Project;

typedef void (*FrameOp)(void *arg1, void *arg2);
typedef void (*EndOp)(void *arg, void *arg2);
typedef struct animation Animation;

typedef struct animation {
    FrameOp frame_op;
    EndOp end_op;
    void *arg1;
    void *arg2;
    int frame_ctr;

    /* Linked list */
    Animation *next;
    Animation *prev;
} Animation;

Animation *project_queue_animation(
    FrameOp frame,
    EndOp end,
    void *arg1,
    void *arg2,
    int ctr_init);

void project_animations_do_frame();
void project_dequeue_animation(Animation *a);
void project_destroy_animations(Project *proj);
#endif
