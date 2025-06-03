/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    animation.h

    * GUI-related operations that must occur every frame
    * each animation has a timer that is decremented every frame
*****************************************************************************************************************/

#ifndef JDAW_ANIMATION_H
#define JDAW_ANIMATION_H

typedef struct session Session;

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

Animation *session_queue_animation(
    FrameOp frame,
    EndOp end,
    void *arg1,
    void *arg2,
    int ctr_init);

void session_animations_do_frame();
void session_dequeue_animation(Animation *a);
void session_destroy_animations(Session *session);
#endif
