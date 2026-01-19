/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "thread_safety.h"

static pthread_t MAIN_THREAD_ID = 0;
static pthread_t DSP_THREAD_ID = 0;
static pthread_t PLAYBACK_THREAD_ID = 0;

static JDAW_THREAD_LOCAL pthread_t CURRENT_THREAD_ID = 0;

void set_thread_id(enum jdaw_thread index)
{
    pthread_t self = pthread_self();
    switch(index) {
    case JDAW_THREAD_MAIN:
	MAIN_THREAD_ID = self;
	break;
    case JDAW_THREAD_DSP:
	DSP_THREAD_ID = self;
	break;
    case JDAW_THREAD_PLAYBACK:
	PLAYBACK_THREAD_ID = self;
	break;
    default:
	break;
    }
    CURRENT_THREAD_ID = self;
}

pthread_t *get_thread_addr(enum jdaw_thread index)
{
    switch(index) {
    case JDAW_THREAD_MAIN:
	return &MAIN_THREAD_ID;
    case JDAW_THREAD_DSP:
	return &DSP_THREAD_ID;
    case JDAW_THREAD_PLAYBACK:
	return &PLAYBACK_THREAD_ID;
    default:
	return NULL;
    }
}

const char *get_thread_name(enum jdaw_thread thread)
{
    switch (thread) {
    case JDAW_THREAD_MAIN:
	return "main";
	break;
    case JDAW_THREAD_DSP:
	return "dsp";
	break;
    case JDAW_THREAD_PLAYBACK:
	return "playback";
	break;
    default:
	return "other";
    }
}

const char *get_current_thread_name()
{
    if (CURRENT_THREAD_ID == PLAYBACK_THREAD_ID) {
	return "playback";
    }
    if (CURRENT_THREAD_ID == DSP_THREAD_ID) {
	return "dsp";
    }
    if (CURRENT_THREAD_ID == MAIN_THREAD_ID) {
	return "main";
    }

    return "other";
    /* Session *session = session_get(); */
    /* pthread_t id = pthread_self(); */
    /* if (id == session->main_thread) { */
    /* 	return "main"; */
    /* } else if (id == session->dsp_thread) { */
    /* 	return "dsp"; */
    /* } else if (id == session->playback_thread) { */
    /* 	return "playback"; */
    /* } else { */
    /* 	return "other"; */
    /* } */
}

/* bool current_thread_main() */
/* { */
/*     if (pthread_self() == MAIN_THREAD_ID) { */
/* 	return true; */
/*     } */
/*     return false; */
/* } */

/* bool current_thread_dsp() */
/* { */
/*     if (pthread_self() == DSP_THREAD_ID) { */
/* 	return true; */
/*     } */
/*     return false; */
/* } */

bool on_thread(enum jdaw_thread thread_index)
{
    pthread_t id = pthread_self();
    if (thread_index == JDAW_THREAD_MAIN && id == MAIN_THREAD_ID)
	return true;
    else if (thread_index == JDAW_THREAD_DSP && id == DSP_THREAD_ID)
	return true;
    else
	return false;
}

enum jdaw_thread current_thread()
{
    pthread_t id = pthread_self();
    if (id == MAIN_THREAD_ID) {
	return JDAW_THREAD_MAIN;
    } else if (id == DSP_THREAD_ID) {
	return JDAW_THREAD_DSP;
    }else if (id == PLAYBACK_THREAD_ID) {
	return JDAW_THREAD_PLAYBACK;
    } else {
	return NUM_JDAW_THREADS;
    }
}
