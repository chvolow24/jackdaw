#ifndef JDAW_TRANSPORT_H
#define JDAW_TRANSPORT_H

#include <stdint.h>
#include "project.h"

#define RING_BUF_LEN_FFT_CHUNKS 5
#define TIMESPEC_TO_MS(ts) ((double)ts.tv_sec * 1000.0f + (double)ts.tv_nsec / 1000000.0f)
#define TIMESPEC_DIFF_MS(ts_end, ts_start) (((double)(ts_end.tv_sec - ts_start.tv_sec) * 1000.0f) + ((double)(ts_end.tv_nsec - ts_start.tv_nsec) / 1000000.0f))

/* Buf for each channel must be freed after done */
typedef struct queued_buf {
    int channels;
    float *buf[2];
    int32_t len_sframes;
    int32_t play_index;
    int32_t play_after_sframes;
    bool free_when_done;
} QueuedBuf;

void transport_record_callback(void* user_data, uint8_t *stream, int len);
void transport_playback_callback(void* user_data, uint8_t* stream, int len);
void transport_start_playback();
void transport_stop_playback();
void transport_start_recording();
void transport_stop_recording();

/* Account for continuous-playback ramifications of calls to timeline_set_play_position(); */
void transport_execute_playhead_jump(Timeline *tl, int32_t new_pos);

void transport_set_mark(Timeline *tl, bool in);
void transport_goto_mark(Timeline *tl, bool in);

void transport_grab_ungrab_at_point();
void transport_recording_update_cliprects();

void create_clip_buffers(Clip *clip, uint32_t len_sframes);
void copy_pd_buf_to_clip(Clip *clip);
#endif
