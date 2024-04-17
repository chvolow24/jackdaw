#ifndef JDAW_TRANSPORT_H
#define JDAW_TRANSPORT_H

#include <stdint.h>

void transport_record_callback(void* user_data, uint8_t *stream, int len);
void transport_playback_callback(void* user_data, uint8_t* stream, int len);
void transport_start_playback();
void transport_stop_playback();
void transport_start_recording();
void transport_stop_recording();
#endif
