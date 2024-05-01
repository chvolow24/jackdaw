#ifndef JDAW_TRANSPORT_H
#define JDAW_TRANSPORT_H

#include <stdint.h>
#include "project.h"
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
#endif
