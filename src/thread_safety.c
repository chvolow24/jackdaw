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
