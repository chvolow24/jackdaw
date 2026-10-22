#ifndef JDAW_SHARED_ARRAY_H
#define JDAW_SHARED_ARRAY_H

#include <stdint.h>
#include "dev.h"
#include "thread_safety.h"


typedef struct shared_array {
    void *_Atomic data;
    size_t el_size;
    enum jdaw_thread user_threads[NUM_JDAW_THREADS];
    int num_user_threads;
} SharedArray;

/* opaque to clients */
struct sa_data {
    SharedArray *sa;
    int32_t __iter;
    void *__data;
    void *__array;
    int32_t len;
};

void sa_init(
    SharedArray *sa,
    size_t el_size,
    enum jdaw_thread *user_threads,
    int num_user_threads);

/* Array *will* be modified */
struct sa_data sa_get_wr(SharedArray *sa);

/* Array will not be modified */
const struct sa_data sa_get_rdonly(SharedArray *sa);

void sa_set(const struct sa_data *old_data, void *new_array, int32_t new_len);

/* Copy opaque array into (typed stack array) dst */
void sa_copy(const struct sa_data *sd, void *dst, int32_t len);

/* Return the next member of the SA */
const void *const sa_iter(struct sa_data *data);

/* Return ptr to item at index */
const void *const sa_at(const struct sa_data *data, int32_t index);

void sa_deinit(SharedArray *sa);

#endif
