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

int num_calls_to_on_thread = 0;
double time_on_pthread_self = 0.0;
double time_on_other = 0.0;
bool on_thread(enum jdaw_thread thread_index)
{
    /* TODO: use thread local global variable for thread id rather than calling pthread_self() */

    pthread_t id;
    num_calls_to_on_thread++;
    clock_t c = clock();
    id = pthread_self();
    time_on_pthread_self += ((double)(clock() - c) / CLOCKS_PER_SEC);

    c = clock();
    id = CURRENT_THREAD_ID;
    time_on_other += ((double)(clock() - c) / CLOCKS_PER_SEC);

    if (num_calls_to_on_thread > 50000) {
	fprintf(stderr, "OLD: %f\nNEW:%f\n", time_on_pthread_self, time_on_other);
	exit(0);
	
    }
    
    
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
