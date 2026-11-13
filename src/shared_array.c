#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "log.h"
#include "session.h"
#include "shared_array.h"
#include "thread_safety.h"

#ifdef TESTBUILD
void sa_debug_guard_thread_access(SharedArray *sa)
{
    enum jdaw_thread current = current_thread();
    if (current == JDAW_THREAD_MAIN) return;
    for (int i=0; i<sa->num_user_threads; i++) {
        if (sa->user_threads[i] == current) return;
    }
    /* Current thread not found in list */
    error_exit(
        "Thread %s tried unauthorized access to \"%s\"\n",
        get_thread_name(current),
        sa->__name);             
}
#else
#define sa_debug_guard_thread_access(sa)
#endif

void sa_init_real(
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
    sa_debug_guard_thread_access(sa);
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
    sa_debug_guard_thread_access(sa);
    struct sa_data ret = {0};
    #ifdef TESTBUILD
    if (sa->__thread_unclosed_write[current_thread()]) {
        error_exit("call to sa_get_wr on unclosed sa in thread %s. Prev bt: \n%s\n", get_current_thread_name(), sa->prev_wr_bt[current_thread()]);
        return ret;
    }
    snprint_backtrace(sa->prev_wr_bt[current_thread()], SHARED_ARRAY_BACKTRACE_MAX_LEN - 1);
    #endif

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

void sa_copy_rdonly(const struct sa_data *sd, void *dst, int32_t len)
{
    memcpy(dst, sd->__array, len * sd->sa->el_size);
}

/* If exit early after call to sa_get_wr, use this to signal that
   we no longer intend to sa_set */
void sa_discard(const struct sa_data *old_data)
{
    old_data->sa->__thread_unclosed_write[current_thread()] = false;
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
    if (new_len > 0) {
        memcpy((uint8_t *)new_data + sizeof(int32_t), new_array, old_data->sa->el_size * new_len);
    }
    
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
