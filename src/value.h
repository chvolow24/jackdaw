#ifndef JDAW_VALUE_H
#define JDAW_VALUE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#define JDAW_VAL_CAST(val, src_type, end_type)	    \
    ((src_type) == JDAW_FLOAT ? (end_type)(val).float_v : \
     (src_type) == JDAW_DOUBLE ? (end_type)(val).double_v : \
     (src_type) == JDAW_INT ? (end_type)(val).int_v : \
     (src_type) == JDAW_UINT8 ? (end_type)(val).uint8_v : \
     (src_type) == JDAW_UINT16 ? (end_type)(val).uint16_v : \
     (src_type) == JDAW_UINT32 ? (end_type)(val).uint32_v : \
     (src_type) == JDAW_INT8 ? (end_type)(val).int8_v : \
     (src_type) == JDAW_INT16 ? (end_type)(val).int16_v : \
     (src_type) == JDAW_INT32 ? (end_type)(val).int32_v : \
     (src_type) == JDAW_BOOL ? (end_type)(val).bool_v : \
     0 )

/* #define JDAW_VALPTR_CAST(valptr, type) \ */
/*     ((type) == JDAW_FLOAT ? (valptr)->float_v : \ */
/*      (type) == JDAW_DOUBLE ? (valptr)->double_v : \ */
/*      (type) == JDAW_INT ? (valptr)->int_v : \ */
/*      (type) == JDAW_UINT8 ? (valptr)->uint8_v : \ */
/*      (type) == JDAW_UINT16 ? (valptr)->uint16_v : \ */
/*      (type) == JDAW_UINT32 ? (valptr)->uint32_v : \ */
/*      (type) == JDAW_INT8 ? (valptr)->int8_v : \ */
/*      (type) == JDAW_INT16 ? (valptr)->int16_v : \ */
/*      (type) == JDAW_INT32 ? (valptr)->int32_v : \ */
/*      (type) == JDAW_BOOL ? (valptr)->bool_v : \ */
/*      0 ) */

typedef enum value_type {
    JDAW_FLOAT = 0,
    JDAW_DOUBLE = 1,
    JDAW_INT = 2,
    JDAW_UINT8 = 3,
    JDAW_UINT16 = 4,
    JDAW_UINT32 = 5,
    JDAW_INT8 = 6,
    JDAW_INT16 = 7,
    JDAW_INT32 = 8,
    JDAW_BOOL = 9
} ValType;

typedef union value {
    double double_v;
    float float_v;
    int int_v;
    uint8_t uint8_v;
    uint16_t uint16_v;
    uint32_t uint32_v;
    int8_t int8_v;
    int16_t int16_v;
    int32_t int32_v;
    bool bool_v;
} Value;

Value jdaw_val_from_ptr(void *ptr, ValType vt);
void jdaw_val_set_ptr(void *valptr, ValType vt, Value new_value);
void jdaw_val_set(Value *vp, ValType vt, void *valptr);
void jdaw_val_set_min(Value *vp, ValType vt);
void jdaw_val_set_max(Value *vp, ValType vt);
Value jdaw_val_add(Value a, Value b, ValType vt);
/* a minus b */
Value jdaw_val_sub(Value a, Value b, ValType vt);
Value jdaw_val_mult(Value a, Value b, ValType vt);
Value jdaw_val_div(Value a, Value b, ValType vt);
Value jdaw_val_scale(Value a, double scalar, ValType vt);
double jdaw_val_div_double(Value a, Value b, ValType vt);
void jdaw_valptr_set_str(char *dst, size_t dstsize, void *value, ValType type, int decimal_places);
void jdaw_val_set_str(char *dst, size_t dstsize, Value value, ValType type, int decimal_places);
bool jdaw_val_less_than(Value a, Value b, ValType type);
bool jdaw_val_is_zero(Value a, ValType type);
bool jdaw_val_equal(Value a, Value b, ValType type);
bool jdaw_val_sign_match(Value a, Value b, ValType type);
void jdaw_val_to_str(char *dst, size_t dstsize, Value v, ValType type, int decimal_places);
Value jdaw_val_from_str(const char *str, ValType type);

bool jdaw_val_in_range(Value test, Value min, Value max, ValType type);
Value jdaw_val_negate(Value a, ValType vt);


void jdaw_val_serialize(FILE *f, Value v, ValType type);
Value jdaw_val_deserialize(FILE *f);

#endif
