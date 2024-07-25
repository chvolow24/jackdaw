#ifndef JDAW_VALUE_H
#define JDAW_VALUE_H

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#define JDAW_VAL_CAST(val, type) \
    ((type) == JDAW_FLOAT ? (val).float_v : \
     (type) == JDAW_DOUBLE ? (val).double_v : \
     (type) == JDAW_INT ? (val).int_v : \
     (type) == JDAW_UINT8 ? (val).uint8_v : \
     (type) == JDAW_UINT16 ? (val).uint16_v : \
     (type) == JDAW_UINT32 ? (val).uint32_v : \
     (type) == JDAW_INT8 ? (val).int8_v : \
     (type) == JDAW_INT16 ? (val).int16_v : \
     (type) == JDAW_INT32 ? (val).int32_v : \
     (type) == JDAW_BOOL ? (val).bool_v : \
     0 )

#define JDAW_VALPTR_CAST(valptr, type) \
    ((type) == JDAW_FLOAT ? (valptr)->float_v : \
     (type) == JDAW_DOUBLE ? (valptr)->double_v : \
     (type) == JDAW_INT ? (valptr)->int_v : \
     (type) == JDAW_UINT8 ? (valptr)->uint8_v : \
     (type) == JDAW_UINT16 ? (valptr)->uint16_v : \
     (type) == JDAW_UINT32 ? (valptr)->uint32_v : \
     (type) == JDAW_INT8 ? (valptr)->int8_v : \
     (type) == JDAW_INT16 ? (valptr)->int16_v : \
     (type) == JDAW_INT32 ? (valptr)->int32_v : \
     (type) == JDAW_BOOL ? (valptr)->bool_v : \
     0 )
    

typedef enum value_type {
    JDAW_FLOAT,
    JDAW_DOUBLE,
    JDAW_INT,
    JDAW_UINT8,
    JDAW_UINT16,
    JDAW_UINT32,
    JDAW_INT8,
    JDAW_INT16,
    JDAW_INT32,
    JDAW_BOOL,
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
#endif
