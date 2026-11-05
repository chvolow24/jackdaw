#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "log.h"
#include "session.h"
#include "shared_array.h"
#include "thread_safety.h"

void sa_init(
    SharedArray *sa,
    size_t el_size,
    enum jdaw_thread *user_threads,
    int num_user_threads)
{
    sa->el_size = el_size;
    memcpy(sa->user_threads, user_threads, num_user_threads * sizeof(enum jdaw_thread));
    sa->num_user_threads = num_user_threads;
    sa->data = malloc(sizeof(int32_t));
    *(int32_t *)sa->data = 0;
}

static void schedule_free(SharedArray *sa, void *old_data)
{
    uint32_t epochs[sa->num_user_threads];
    for (int i=0; i<sa->num_user_threads; i++) {
        epochs[i] = thread_epoch(sa->user_threads[i]);
    }
    session_schedule_free(
        old_data,
        sa->user_threads,
        epochs,
        sa->num_user_threads);
}

struct sa_data sa_get_rdonly(SharedArray *sa)
{
    struct sa_data ret = {0};
    ret.__data = atomic_load_explicit(&sa->data, memory_order_acquire);
    ret.len = *(int32_t *)(ret.__data);
    ret.__array = ret.__data + sizeof(int32_t);
    ret.__rdonly = true;
    ret.sa = sa;
    return ret;
}

struct sa_data sa_get_wr(SharedArray *sa)
{
    struct sa_data ret = {0};
    if (sa->__thread_unclosed_write[current_thread()]) {
        char bt[4096] = {0};
        snprint_backtrace(bt, 4095);
        error_exit("%s\ncall to sa_get_wr on unclosed sa in thread %s\n\n", bt, get_current_thread_name());
        return ret;
    }

    ret.__data = atomic_load_explicit(&sa->data, memory_order_acquire);
    ret.len = *(int32_t *)(ret.__data);
    ret.__array = ret.__data + sizeof(int32_t);
    ret.__rdonly = false;
    ret.sa = sa;
    sa->__thread_unclosed_write[current_thread()] = true;
    return ret;
}

void sa_copy(const struct sa_data *sd, void *dst, int32_t len)
{
    if (sd->__rdonly) {
        log_tmp(LOG_WARN, "called sa_copy on a read-only shared array data\n");
    }
    memcpy(dst, sd->__array, len * sd->sa->el_size);
}


void sa_set(const struct sa_data *old_data, void *new_array, int32_t new_len)
{
    if (!old_data) {
        log_tmp(LOG_ERROR, "shared_array_set: old sa_data MUST be returned to SA\n");
        return;
    }
    if (old_data->__rdonly) {
        log_tmp(LOG_ERROR, "attempting to 'set' read-only shared array data\n");
        return;
    }
    void *new_data = malloc(sizeof(int32_t) + old_data->sa->el_size * new_len);
    memcpy(new_data, &new_len, sizeof(int32_t));
    memcpy((uint8_t *)new_data + sizeof(int32_t), new_array, old_data->sa->el_size * new_len);
    
    atomic_store_explicit(&old_data->sa->data, new_data, memory_order_release);
    schedule_free(old_data->sa, old_data->__data);
    old_data->sa->__thread_unclosed_write[current_thread()] = false;
}


void sa_push(SharedArray *sa, void *item)
{
    void *old_data = atomic_load_explicit(&sa->data, memory_order_acquire);
    int32_t len = *(int32_t *)old_data;    
    void *array = (uint8_t *)old_data + sizeof(int32_t);
    void *new_data = malloc(sizeof(int32_t) + (len + 1) * sa->el_size);
    uint8_t *new_array = (uint8_t *)new_data + sizeof(int32_t);
    memcpy(new_array, array, sa->el_size * len);
    memcpy((uint8_t *)new_array + (sa->el_size * len), item, sa->el_size);
    *(int32_t *)new_data = len + 1;
    
    atomic_store_explicit(&sa->data, new_data, memory_order_release);
    
    schedule_free(sa, old_data);
}

int32_t sa_len(SharedArray *sa)
{
    void *data = atomic_load_explicit(&sa->data, memory_order_acquire);
    int32_t len = *(int32_t *)data;
    return len;
}

/* To be placed in a "while" loop */
const void *const sa_iter(struct sa_data *data)
{
    if (data->__iter >= data->len) {
        data->__iter = 0;
        return NULL;
    }
    return (uint8_t *)data->__array + data->__iter++ * data->sa->el_size;
}

const void *const sa_at(const struct sa_data *data, int32_t index)
{
    return (uint8_t *)data->__array + index * data->sa->el_size;
}


void sa_deinit(SharedArray *sa)
{
    MAIN_THREAD_ONLY(sa_deinit);
    for (int i=0; i<sa->num_user_threads; i++) {
        enum jdaw_thread t = sa->user_threads[i];
        if (thread_is_active(t) && t != JDAW_THREAD_MAIN) {
            log_tmp(LOG_ERROR, "Attempt to deinit shared array with active user thread (%s)\n", get_thread_name(t));
        }
    }
    free(sa->data);
}
