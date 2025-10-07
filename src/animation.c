/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdlib.h>
#include "animation.h"
#include "session.h"

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


/* #include <execinfo.h> */

/* struct abt { */
/*     Animation *a; */
/*     char **bt; */
/*     size_t num_bt_els; */
/* }; */

/* #define ABTS_MAX 1000 */
/* struct abt abts[ABTS_MAX]; */
/* struct abt alloc_abts[ABTS_MAX]; */
/* uint32_t num_abts; */
/* uint32_t num_alloc_abts; */

/* static void get_bt_alloc(Animation *a) */
/* { */
/*     if (num_alloc_abts == ABTS_MAX) { */
/* 	fprintf(stderr, "Already reached max logged abts\n"); */
/* 	return; */
/*     } */
/*     void *arr[255]; */
/*     int num = backtrace(arr, 255); */
/*     char **bt_syms = backtrace_symbols(arr, num); */
/*     alloc_abts[num_alloc_abts].a = a; */
/*     alloc_abts[num_alloc_abts].bt = bt_syms; */
/*     alloc_abts[num_alloc_abts].num_bt_els = num; */
/*     num_alloc_abts++; */

/* } */

/* static void get_bt(Animation *a) */
/* { */
/*     if (num_abts == ABTS_MAX) { */
/* 	fprintf(stderr, "Already reached max logged abts\n"); */
/* 	return; */
/*     } */
/*     void *arr[255]; */
/*     int num = backtrace(arr, 255); */
/*     char **bt_syms = backtrace_symbols(arr, num); */
/*     abts[num_abts].a = a; */
/*     abts[num_abts].bt = bt_syms; */
/*     abts[num_abts].num_bt_els = num; */
/*     num_abts++; */
/* } */

/* static void print_bt(Animation *a) */
/* { */
/*     /\* fprintf(stderr, "Prev freed:\n"); *\/ */
/*     for (uint32_t i=0; i<num_abts; i++) { */
/* 	struct abt* abt = abts + i; */
/* 	/\* fprintf(stderr, "\t%p\n", abt->a); *\/ */
/* 	if (abt->a == a) { */
/* 	    fprintf(stderr, "\tANIMATION %p ALREADY FREED! Backtrace:\n", a); */
/* 	    for (int j=0; j<abt->num_bt_els; j++) { */
/* 		fprintf(stderr, "\t\t->%s\n", abt->bt[j]); */
/* 	    } */
/* 	    breakfn(); */
/* 	    break; */
/* 	} */
/*     } */
/* } */

/* static void print_bt_alloc(Animation *a) */
/* { */
/*     for (uint32_t i=0; i<num_alloc_abts; i++) { */
/* 	struct abt* abt = alloc_abts + i; */
/* 	/\* fprintf(stderr, "\t%p\n", abt->a); *\/ */
/* 	if (abt->a == a) { */
/* 	    fprintf(stderr, "\tANIMATION %p allocated at:\n", a); */
/* 	    for (int j=0; j<abt->num_bt_els; j++) { */
/* 		fprintf(stderr, "\t\t->%s\n", abt->bt[j]); */
/* 	    } */
/* 	    breakfn(); */
/* 	    break; */
/* 	} */
/*     } */
/* } */



static void animation_destroy(Animation *a)
{
    free(a);
}

Animation *session_queue_animation(
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

    /* get_bt_alloc(new); */

    /* pthread_mutex_lock(&proj->animation_lock); */
    Session *session = session_get();
    Animation *node = session->animations;
    if (!node) {
	session->animations = new;
	/* pthread_mutex_unlock(&proj->animation_lock); */
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



void session_dequeue_animation(Animation *a)
{
    /* print_bt(a); */
    /* /\* print_bt_alloc(a); *\/ */
    /* get_bt(a); */
    Session *session = session_get();
    if (!a->prev) {
	session->animations = a->next;
    } else {
	a->prev->next = a->next;
    }
    if (a->next) {
	a->next->prev = a->prev;
    }
    if (a->label) {
	a->label->animation = NULL;
    }
    animation_destroy(a);

}


void session_animations_do_frame()
{
    /* pthread_mutex_lock(&proj->animation_lock); */
    Session *session = session_get();
    Animation *a = session->animations;
    while (a) {
	if (a->frame_op)
	    a->frame_op(a->arg1, a->arg2);
	a->frame_ctr--;
	if (a->frame_ctr == 0) {
	    if (a->end_op)
		a->end_op(a->arg1, a->arg2);
	    /* if (a->prev) { */
	    /* 	a->prev->next = a->next; */
	    /* 	if (a->next) { */
	    /* 	    a->next->prev = a->prev; */
	    /* 	} */
	    /* } else { */
	    /* 	proj->animations = a->next; */
	    /* 	if (a->next) { */
	    /* 	    a->next->prev = NULL; */
	    /* 	} */
	    /* } */
	    Animation *next  = a->next;
	    session_dequeue_animation(a);
	    /* animation_destroy(a); */
	    a = next;	    
	} else {
	    a = a->next;
	}

    }
    /* pthread_mutex_unlock(&proj->animation_lock); */
}

void session_destroy_animations(Session *session)
{
    Animation *a = session->animations;
    while (a) {
	Animation *next = a->next;
	session_dequeue_animation(a);
	/* animation_destroy(a); */
	a = next;
    }
}
