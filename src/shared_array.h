#ifndef JDAW_SHARED_ARRAY_H
#define JDAW_SHARED_ARRAY_H

#include <stdint.h>
#include "thread_safety.h"


typedef struct shared_array {
    void *_Atomic data;
    size_t el_size;
    enum jdaw_thread user_threads[NUM_JDAW_THREADS];
    int num_user_threads;
    bool __thread_unclosed_write[NUM_JDAW_THREADS];

} SharedArray;

struct sa_data {
    SharedArray *sa;
    int32_t len;
    int32_t __iter; /* incremented in sa_iter */
    void *__data;
    void *__array;
    bool __rdonly;
};

void sa_init(
    SharedArray *sa,
    size_t el_size,
    enum jdaw_thread *user_threads,
    int num_user_threads);

/* Get data for writing. Must be copied before modifying
   with sa_copy. Commit changed with sa_set */
struct sa_data sa_get_wr(SharedArray *sa);

/* Get a copy for reading */
struct sa_data sa_get_rdonly(SharedArray *sa);

/* Swap the main shared array pointer */
void sa_set(const struct sa_data *old_data, void *new_array, int32_t new_len);

/* If exit early after call to sa_get_wr, use this to signal that
   we no longer intend to sa_set */
void sa_discard(const struct sa_data *old_data);

/* Copy opaque array into (typed stack array) dst */
void sa_copy(const struct sa_data *sd, void *dst, int32_t len);

/* Return the next member of the SA. Prefer sa_at */
const void *const sa_iter(struct sa_data *data);

/* Return ptr to item at index */
const void *const sa_at(const struct sa_data *data, int32_t index);

void sa_deinit(SharedArray *sa);

int32_t sa_len(SharedArray *sa);

#endif
