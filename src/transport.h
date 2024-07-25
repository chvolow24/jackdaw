#ifndef JDAW_TRANSPORT_H
#define JDAW_TRANSPORT_H


#include <stdint.h>
#include "project.h"

#define RING_BUF_LEN_FFT_CHUNKS 2
#define TIMESPEC_TO_MS(ts) ((double)ts.tv_sec * 1000.0f + (double)ts.tv_nsec / 1000000.0f)
#define TIMESPEC_DIFF_MS(ts_end, ts_start) (((double)(ts_end.tv_sec - ts_start.tv_sec) * 1000.0f) + ((double)(ts_end.tv_nsec - ts_start.tv_nsec) / 1000000.0f))

void transport_record_callback(void* user_data, uint8_t *stream, int len);
void transport_playback_callback(void* user_data, uint8_t* stream, int len);
void transport_start_playback();
void transport_stop_playback();
void transport_start_recording();
void transport_stop_recording();

void transport_set_mark(Timeline *tl, bool in);
void transport_goto_mark(Timeline *tl, bool in);

void transport_grab_ungrab_at_point();
void transport_recording_update_cliprects();

void create_clip_buffers(Clip *clip, uint32_t len_sframes);
void copy_pd_buf_to_clip(Clip *clip);
#endif
