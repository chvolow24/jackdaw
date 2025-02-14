#ifndef JDAW_TYPE_SERIALIZE_H
#define JDAW_TYPE_SERIALIZE_H

#include <stdint.h>
#include <math.h>
#include <stdio.h>


uint16_t uint16_fromstr_le(char *buf);
uint32_t uint32_fromstr_le(char *buf);
uint64_t uint64_fromstr_le(char *buf);
void uint16_tostr_le(uint16_t val, char *buf);
void uint32_tostr_le(uint32_t val, char *buf);
void uint64_tostr_le(uint64_t val, char *buf);

/* Fixed 5-byte size */
void float_tostr40_le(double ts, char *buf);
/* Fixed 5-byte size */
double float_fromstr40_le(char *buf);

/* Primary (signed) serializations */

void uint8_ser(FILE *f, uint8_t *val_p);
void uint16_ser_le(FILE *f, uint16_t *val_p);
void uint32_ser_le(FILE *f, uint32_t *val_p);
void uint64_ser_le(FILE *f, uint64_t *val_p);

uint8_t uint8_deser(FILE *f);
uint16_t uint16_deser_le(FILE *f);
uint32_t uint32_deser_le(FILE *f);
uint64_t uint64_deser_le(FILE *f);

/* Secondary (signed) serializations */

void int8_ser(FILE *f, int8_t *v_p);
void int16_ser_le(FILE *f, int16_t *v_p);
void int32_ser_le(FILE *f, int32_t *v_p);
void int64_ser_le(FILE *f, int64_t *v_p);
void int_ser_le(FILE *f, int *v_p);

int8_t int8_deser(FILE *f);
int16_t int16_deser_le(FILE *f);
int32_t int32_deser_le(FILE *f);
int64_t int64_deser_le(FILE *f);
int int_deser_le(FILE *f);

/* Floats */

void float_ser40_le(FILE *f, double val);
double float_deser40_le(FILE *f);

#endif
