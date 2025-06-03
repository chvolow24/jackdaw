/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "thread_safety.h"

pthread_t MAIN_THREAD_ID = 0;
pthread_t DSP_THREAD_ID = 0;
JDAW_THREAD_LOCAL pthread_t CURRENT_THREAD_ID = 0;


const char *get_thread_name()
{
    pthread_t id = pthread_self();
    if (id == MAIN_THREAD_ID) {
	return "main";
    } else if (id == DSP_THREAD_ID) {
	return "dsp";
    } else {
	return "other";
    }
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
    /* pthread_t id = CURRENT_THREAD_ID;    */
    pthread_t id = pthread_self();
    if (thread_index == JDAW_THREAD_MAIN && id == MAIN_THREAD_ID)
	return true;
    else if (thread_index == JDAW_THREAD_DSP && id == DSP_THREAD_ID)
	return true;
    else
	return false;
}

/* enum jdaw_thread jdaw_current_thread() */
/* { */
/*     pthread_t id = pthread_self(); */
/*     if (id == MAIN_THREAD_ID) { */
/* 	return JDAW_THREAD_MAIN; */
/*     } else if (id == DSP_THREAD_ID) { */
/* 	return JDAW_THREAD_DSP; */
/*     } else { */
/* 	return JDAW_THREAD_OTHER; */
/*     } */
/* } */
