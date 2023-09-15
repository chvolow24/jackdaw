#ifndef JDAW_WAV_H
#define JDAW_WAV_H

#include <stdint.h>

void write_wav(const char *fname, int16_t *samples, uint32_t num_samples, uint16_t bits_per_sample, uint8_t channels);

#endif