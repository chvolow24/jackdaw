#include "log.h"
#include "piano_roll.h"
#include "project.h"
#include "session.h"
#include "session_endpoint_ops.h"
#include "spsc_lfqueue.h"

#define INSTRUMENT_MONITOR_WAIT_LOOP_USECONDS 100

static _Atomic bool cancel_monitoring = false;

static void *instrument_monitor_threadfn(void *arg)
{
    (void)arg;
    set_thread_id(JDAW_THREAD_INSTRUMENT);

    struct sched_param sched;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &sched);
    fprintf(stderr, "ACTUAL PRI: %d policy %s\n", sched.sched_priority, policy == SCHED_RR ? "RR" : policy == SCHED_FIFO ? "FIFO" : "other");
    Session *session = session_get();
    int len_sframes = session->proj.chunk_size_sframes;;
    MIDIDevice *d = session->midi_io.monitor_device;
    Synth *s = session->midi_io.monitor_synth;
    Timeline *tl = ACTIVE_TL;
    if (!d || !s) return NULL;

    float LR[len_sframes * 2];
    memset(LR, 0, sizeof(LR));
    
    while (lfqueue_try_enqueue(
               &session->playback.instrument_monitor_lfqueue,
               LR,
               len_sframes * 2) == LFQUEUE_SUCCESS) {};
    fprintf(stderr, "ENTERING!\n");
    while (!atomic_load_explicit(&cancel_monitoring, memory_order_relaxed)) {

        session_run_thread_callbacks(JDAW_THREAD_INSTRUMENT);
        /* fprintf(stderr, "last iter time: %ld\n", clock() - c); */
        /* c = clock(); */
        midi_device_read(d);
        float playspeed = session->playback.play_speed;
        if (session->piano_roll) {
            piano_roll_feed_midi(d->buffer, d->num_unconsumed_events);
        }
        synth_feed_midi(s, d->buffer, d->num_unconsumed_events, 0, true);
        if (d->current_clip && d->current_clip->recording) {
            midi_device_output_chunk_to_clip(d, 1);
            d->current_clip->len_sframes += len_sframes;
        }
        d->num_unconsumed_events = 0;
        if (fabs(playspeed) < 1e-6 || !session->playback.playing) playspeed = 1.0f;

        float L[len_sframes];
        float R[len_sframes];
        memset(L, 0, len_sframes * sizeof(float));
        memset(R, 0, len_sframes * sizeof(float));
        synth_add_buf(s, L, R, len_sframes, playspeed, false, 0); /* TL Pos ignored */
        for (int i=0; i<len_sframes * 2; i+=2) {
            LR[i] = L[i / 2];
            LR[i+1] = R[i / 2];
        }
        lfqueue_wait_enqueue(
            &session->playback.instrument_monitor_lfqueue,
            LR,
            len_sframes * 2,
            INSTRUMENT_MONITOR_WAIT_LOOP_USECONDS,
            0,
            &cancel_monitoring);
        /* lfqueue_wait_enqueue( */
        /*     &tl->monitoring_instrument_R, */
        /*     R, */
        /*     len_sframes, */
        /*     INSTRUMENT_MONITOR_WAIT_LOOP_USECONDS, */
        /*     0, */
        /*     &cancel_monitoring); */
        session_do_ongoing_changes(session, JDAW_THREAD_INSTRUMENT);
        session_flush_val_changes(session, JDAW_THREAD_INSTRUMENT);
        session_flush_callbacks(session, JDAW_THREAD_INSTRUMENT);
        /* fprintf(stderr, "Done ongoing and val changes\n"); */
    }
    return NULL;
}

void instrument_monitor_start()
{
    Session *session = session_get();
    atomic_store_explicit(&cancel_monitoring, false, memory_order_relaxed);

    pthread_attr_t attr;
    int sched_policy = SCHED_FIFO;
    int ret;
    if ((ret = pthread_attr_init(&attr)) != 0) {
	fprintf(stderr, "pthread_attr_init: %s\n", strerror(ret));
    }
    if ((ret = pthread_attr_setschedpolicy(&attr, sched_policy)) != 0) {
	fprintf(stderr, "pthread_attr_setschedpolicy: %s\n", strerror(ret));
    }
    int priority_max = sched_get_priority_max(sched_policy);
    if (priority_max < 0) {
        perror("sched_get_priority_max");
    }
    int priority_min = sched_get_priority_min(sched_policy);
    if (priority_min < 0) {
        perror("sched_get_priority_max");
    }
    int priority = priority_min + (priority_max - priority_min) * 0.8;
    fprintf(stderr, "PRI range: %d, %d == %d\n", priority_min, priority_max, priority);
    struct sched_param instrument_sched;
    
    instrument_sched.sched_priority = priority;
    if ((ret = pthread_attr_setschedparam(&attr, &instrument_sched)) != 0) {
	fprintf(stderr, "pthread_attr_setschedparam: %s\n", strerror(ret));
    }

    if ((ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) != 0) {
        fprintf(stderr, "pthread_attr_setinheritsched: %s\n", strerror(ret));
    }
    thread_set_active(JDAW_THREAD_INSTRUMENT);
    if ((ret = pthread_create(get_thread_addr(JDAW_THREAD_INSTRUMENT), &attr, instrument_monitor_threadfn, NULL)) != 0) {
        log_tmp(LOG_WARN, "pthread_create failed to create instrument monitor thread with sched pri %d: %s\n", priority, strerror(ret));        
        if ((ret = pthread_create(get_thread_addr(JDAW_THREAD_INSTRUMENT), NULL, instrument_monitor_threadfn, NULL)) != 0) {
            fprintf(stderr, "pthread_create fallback failed to create instrument monitor: %s\n", strerror(ret));
            exit(1);
        }
    }
    pthread_attr_destroy(&attr);
    usleep(1000);
    audioconn_start_playback(session->audio_io.playback_conn);
}

void instrument_monitor_stop()
{
    fprintf(stderr, "Stop monitoring\n");
    atomic_store_explicit(&cancel_monitoring, true, memory_order_relaxed);
    pthread_join(*get_thread_addr(JDAW_THREAD_INSTRUMENT), NULL);
    thread_set_inactive(JDAW_THREAD_INSTRUMENT);
    /* audioconn_stop_playback(session_get()->audio_io.playback_conn); */
}
