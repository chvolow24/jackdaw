#include <semaphore.h>
#include "project.h"

int pd_jackdaw_shm_init();
void pd_signal_termination_of_jackdaw();
void *pd_jackdaw_record_on_thread(void *arg);
