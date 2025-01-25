#include "thread_safety.h"

extern pthread_t MAIN_THREAD_ID;
extern pthread_t DSP_THREAD_ID;
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
    /* TODO: use thread local global variable for thread id rather than calling pthread_self() */
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
