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
    animation.c

    * GUI-related operations that must occur every frame
    * each animation has a timer that is decremented every frame
 *****************************************************************************************************************/

#include <stdlib.h>
#include "animation.h"
#include "project.h"
extern Project *proj;

static Animation *animation_create(
    FrameOp frame_op,
    EndOp end_op,
    void *arg1,
    void *arg2,
    int frame_ctr)
{
    Animation *a = calloc(1, sizeof(Animation));
    a->frame_op = frame_op;
    a->end_op = end_op;
    a->arg1 = arg1;
    a->arg2 = arg2;
    a->frame_ctr = frame_ctr;
    return a;
}

static void animation_destroy(Animation *a)
{
    free(a);
}

Animation *project_queue_animation(
    FrameOp frame_op,
    EndOp end_op,
    void *arg1, void *arg2,
    int ctr_init)
{
    Animation *new = animation_create(
	frame_op,
	end_op,
	arg1, arg2,
	ctr_init);

    /* pthread_mutex_lock(&proj->animation_lock); */
    Animation *node = proj->animations;
    if (!node) {
	proj->animations = new;
	pthread_mutex_unlock(&proj->animation_lock);
	return new;
    }
    while (node->next) {
	node = node->next;
    }
    node->next = new;
    new->prev = node;
    return new;
    /* pthread_mutex_unlock(&proj->animation_lock); */
}

void project_animations_do_frame()
{
    /* pthread_mutex_lock(&proj->animation_lock); */
    Animation *a = proj->animations;
    while (a) {
	if (a->frame_op)
	    a->frame_op(a->arg1, a->arg2);
	a->frame_ctr--;
	if (a->frame_ctr == 0) {
	    if (a->end_op)
		a->end_op(a->arg1, a->arg2);
	    if (a->prev) {
		a->prev->next = a->next;
		if (a->next) {
		    a->next->prev = a->prev;
		}
	    } else {
		proj->animations = a->next;
		if (a->next) {
		    a->next->prev = NULL;
		}
	    }
	    Animation *next  = a->next;
	    animation_destroy(a);
	    a = next;
	    
	} else {
	    a = a->next;
	}

    }
    /* pthread_mutex_unlock(&proj->animation_lock); */
}


void project_dequeue_animation(Animation *a)
{
    if (!a->prev) {
	proj->animations = a->next;
    } else {
	a->prev->next = a->next;
    }
    if (a->next) {
	a->next->prev = a->prev;
    }
    animation_destroy(a);

}

void project_destroy_animations(Project *proj)
{
    Animation *a = proj->animations;
    while (a) {
	Animation *next = a->next;
	animation_destroy(a);
	a = next;
    }
}
