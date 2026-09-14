#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

enum {
    LFQUEUE_SUCCESS,
    LFQUEUE_FULL,
    LFQUEUE_EMPTY,
    LFQUEUE_OVERSIZE_INBUF,
    LFQUEUE_OP_CANCELED,
    LFQUEUE_NUM_STATUS_CODES
};

typedef struct lock_free_queue {
    void *buf;
    size_t el_size;
    int len;
    int wrap_mask;
    _Atomic int write_i;
    _Atomic int read_i;
} LFQueue;

/* Allocate the ring buffer and initialize values */
int lfqueue_init(LFQueue *q, size_t el_size, int len);

/* Free the ring buffer */
void lfqueue_deinit(LFQueue *q);

/* Return LFQUEUE_SUCCESS or an error code < 0 */
int lfqueue_try_enqueue(LFQueue *q, void *restrict inbuf, int inbuf_len);

/* Return LFQUEUE_SUCCESS or an error code < 0 */
int lfqueue_try_dequeue(LFQueue *q, void *restrict dstbuf, int dstbuf_len);

const char *lfqueue_get_errstr(int error_code);

/*
  Wait until space available, then enqueue and exit.
   
  When 'loop_sleep' is 0, waiting is busywaiting; else each
  iteration sleep 'loop_sleep' microseconds.
  
  If 'quit_opt' provided, give up when *quit_opt == true
*/
int lfqueue_wait_enqueue(
    LFQueue *q,
    void *restrict inbuf,
    int inbuf_len,
    useconds_t loop_sleep,
    _Atomic bool *quit_opt);

/*
  Wait data available, then dequeue and exit.
   
  When 'loop_sleep' is 0, waiting is busywaiting; else each
  iteration sleep 'loop_sleep' microseconds.
  
  If 'quit_opt' provided, give up when *quit_opt == true
*/
int lfqueue_wait_dequeue(
    LFQueue *q,
    void *restrict dstbuf,
    int dstbuf_len,
    useconds_t loop_sleep,
    _Atomic bool *quit_opt);

#ifdef SPSC_QUEUE_IMPL

#include <math.h>
#include <stdlib.h>

static const char *lfqueue_errstr[] = {
    "Success",
    "Queue full",
    "Queue empty",
    "Cannot enqueue data longer than half ring buf len",
    "Wait canceled"
};

const char *lfqueue_get_errstr(int error_code)
{
    if (error_code < LFQUEUE_NUM_STATUS_CODES) {
        return lfqueue_errstr[error_code];
    } else {
        return "Unknown error";
    }
}

int lfqueue_init(LFQueue *q, size_t el_size, int len)
{
    if (len < 4) len = 4;
    double lg = log2(len);
    double lgfl = floor(lg);
    if (lg != lgfl) {
        len = pow(2.0, lgfl + 1.0);
    }
    atomic_store(&q->read_i, 0);
    atomic_store(&q->write_i, 0);
    q->buf = malloc(el_size * len);
    q->el_size = el_size;
    q->len = len;
    q->wrap_mask = len - 1;
    return len;
}

void lfqueue_deinit(LFQueue *q)
{
    if (q->buf) free(q->buf);
    q->buf = NULL;
}

int lfqueue_try_enqueue(LFQueue *q, void *restrict inbuf, int inbuf_len)
{
    if (inbuf_len > q->len / 2) {
        return LFQUEUE_OVERSIZE_INBUF;
    }
    int loc_write_i = atomic_load_explicit(&q->write_i, memory_order_relaxed);

    /* Load of read_i must be 'acquire' to prevent writes below from moving
       before the reader releases its index */
    int loc_read_i = atomic_load_explicit(&q->read_i, memory_order_acquire);

    /* Although the reader may read during this operation, we can guarantee
       that the index values here represent the minimum amount of writable
       space in the buffer */

    int avail_to_write =
        loc_write_i < loc_read_i ?
        loc_read_i - loc_write_i - 1 :
        (q->len - loc_write_i) + loc_read_i - 1;
    
    if (avail_to_write < inbuf_len) return LFQUEUE_FULL;
     
    int write_dst = (loc_write_i + inbuf_len) & q->wrap_mask;

    if (loc_write_i < write_dst) {
        memcpy(q->buf + loc_write_i, inbuf, inbuf_len * q->el_size);
    } else {
        int left = q->len - loc_write_i;
        memcpy(q->buf + loc_write_i, inbuf, left * q->el_size);
        memcpy(q->buf, inbuf + left, write_dst * q->el_size);
    }
    atomic_store_explicit(&q->write_i, write_dst, memory_order_release);

    return LFQUEUE_SUCCESS;
}

int lfqueue_try_dequeue(LFQueue *q, void *restrict dstbuf, int dstbuf_len)
{
    int loc_read_i = atomic_load_explicit(&q->read_i, memory_order_relaxed);
    int loc_write_i = atomic_load_explicit(&q->write_i, memory_order_acquire);

    int avail_to_read =
        loc_read_i <= loc_write_i ?
        loc_write_i - loc_read_i :
        (q->len - loc_read_i) + loc_write_i;
    if (avail_to_read < dstbuf_len) {
        return LFQUEUE_EMPTY;
    }

    int read_dst = (loc_read_i + dstbuf_len) & q->wrap_mask;
    if (loc_read_i < read_dst) {
        memcpy(dstbuf, q->buf + loc_read_i, dstbuf_len * q->el_size);
    } else {
        int left = q->len - loc_read_i;
        memcpy(dstbuf, q->buf + loc_read_i, left * q->el_size);
        memcpy(dstbuf + left, q->buf, read_dst * q->el_size);
    }
    atomic_store_explicit(&q->read_i, read_dst, memory_order_release);
    
    return LFQUEUE_SUCCESS;
}

int lfqueue_wait_enqueue(LFQueue *q, void *restrict inbuf, int inbuf_len, useconds_t loop_sleep, _Atomic bool *quit_opt)
{
    if (inbuf_len > q->len / 2) {
        return LFQUEUE_OVERSIZE_INBUF;
    }
    int loc_write_i;
    int loc_read_i;
    int avail_to_write;
    while (1) {
        if (quit_opt && atomic_load_explicit(quit_opt, memory_order_relaxed)) {
            goto canceled;
        }
        loc_write_i = atomic_load_explicit(&q->write_i, memory_order_relaxed);

        /* Load of read_i must be 'acquire' to prevent writes below from moving
           before the reader releases its index */
        loc_read_i = atomic_load_explicit(&q->read_i, memory_order_acquire);

        /* Although the reader may read during this operation, we can guarantee
           that the index values here represent the minimum amount of writable
           space in the buffer */
        avail_to_write =
            loc_write_i < loc_read_i ?
            loc_read_i - loc_write_i - 1 :
            (q->len - loc_write_i) + loc_read_i - 1;
    
        if (avail_to_write < inbuf_len) {
            usleep(loop_sleep);
            continue;
        } else {
            break;
        }
    }
     
    int write_dst = (loc_write_i + inbuf_len) & q->wrap_mask;

    if (loc_write_i < write_dst) {
        memcpy(q->buf + loc_write_i, inbuf, inbuf_len * q->el_size);
    } else {
        int left = q->len - loc_write_i;
        memcpy(q->buf + loc_write_i, inbuf, left * q->el_size);
        memcpy(q->buf, inbuf + left, write_dst * q->el_size);
    }
    atomic_store_explicit(&q->write_i, write_dst, memory_order_release);
    return LFQUEUE_SUCCESS;
canceled:
    return LFQUEUE_OP_CANCELED;
}

int lfqueue_wait_dequeue(LFQueue *q, void *restrict dstbuf, int dstbuf_len, useconds_t loop_sleep, _Atomic bool *quit_opt)
{
    int loc_read_i;
    int loc_write_i;
    int avail_to_read;
    while (1) {
        if (quit_opt && atomic_load_explicit(quit_opt, memory_order_relaxed)) {
            goto canceled;
        }
        loc_read_i = atomic_load_explicit(&q->read_i, memory_order_relaxed);
        loc_write_i = atomic_load_explicit(&q->write_i, memory_order_acquire);

        avail_to_read =
            loc_read_i <= loc_write_i ?
            loc_write_i - loc_read_i :
            (q->len - loc_read_i) + loc_write_i;
        if (avail_to_read < dstbuf_len) {
            usleep(loop_sleep);
            continue;
        } else {
            break;
        }
    }

    int read_dst = (loc_read_i + dstbuf_len) & q->wrap_mask;
    if (loc_read_i < read_dst) {
        memcpy(dstbuf, q->buf + loc_read_i, dstbuf_len * q->el_size);
    } else {
        int left = q->len - loc_read_i;
        memcpy(dstbuf, q->buf + loc_read_i, left * q->el_size);
        memcpy(dstbuf + left, q->buf, read_dst * q->el_size);
    }
    atomic_store_explicit(&q->read_i, read_dst, memory_order_release);
    
    return LFQUEUE_SUCCESS;
canceled:
    return LFQUEUE_OP_CANCELED;
}

#endif
#endif
