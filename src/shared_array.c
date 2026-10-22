#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "session.h"
#include "shared_array.h"

void sa_init(
    SharedArray *sa,
    size_t el_size,
    enum jdaw_thread *user_threads,
    int num_user_threads)
{
    sa->el_size = el_size;
    memcpy(sa->user_threads, user_threads, num_user_threads * el_size);
    sa->num_user_threads = num_user_threads;    
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

const struct sa_data sa_get_rdonly(SharedArray *sa)
{
    struct sa_data ret;
    ret.__data = atomic_load_explicit(&sa->data, memory_order_acquire);
    ret.len = *(int32_t *)(ret.__data);
    ret.__array = ret.__data + sizeof(int32_t);
    return ret;
}

struct sa_data sa_get(SharedArray *sa)
{
    struct sa_data ret;
    ret.__data = atomic_load_explicit(&sa->data, memory_order_acquire);
    ret.len = *(int32_t *)(ret.__data);
    ret.__array = ret.__data + sizeof(int32_t);
    return ret;
}

void sa_copy(const struct sa_data *sd, void *dst, int32_t len)
{
    memcpy(dst, sd->__array, len * sd->sa->el_size);
}


void sa_set(const struct sa_data *old_data, void *new_array, int32_t new_len)
{
    if (!old_data) {
        log_tmp(LOG_ERROR, "shared_array_set: old sa_data MUST be returned to SA\n");
        return;
    }
    void *new_data = malloc(sizeof(int32_t) + old_data->sa->el_size * new_len);
    memcpy(new_data, &new_len, sizeof(int32_t));
    memcpy((uint8_t *)new_data + sizeof(int32_t), new_array, old_data->sa->el_size * new_len);
    
    atomic_store_explicit(&old_data->sa->data, new_data, memory_order_release);
    schedule_free(old_data->sa, old_data->__data);
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

/* To be placed in a "while" loop */
const void *const sa_iter(struct sa_data *data)
{
    if (data->__iter >= data->len) {
        data->__iter = 0;
        return NULL;
    }
    return (uint8_t *)data->__array + data->__iter++ * data->sa->el_size;
}

const void *const sa_at(struct sa_data *data, int32_t index)
{
    return (uint8_t *)data->__array + index * data->sa->el_size;
}


void sa_deinit(SharedArray *sa)
{
    schedule_free(sa, sa->data);
}
