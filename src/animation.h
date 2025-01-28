/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

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
