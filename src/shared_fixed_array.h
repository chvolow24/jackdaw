#ifndef JDAW_SHARED_FIXED_ARRAY_H
#define JDAW_SHARED_FIXED_ARRAY_H

#include "fixed_array.h"
#include "shared_value.h"

#define SHARED_FIXED_ARRAY_NUM_SLOTS 3

SHARED_VALUE_TYPE_DECL_ONLY(SharedDouble64, shared_double64, double64, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedDouble128, shared_double128, double128, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedDouble256, shared_double256, double256, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedDouble512, shared_double512, double512, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedDouble1024, shared_double1024, double1024, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedDouble2048, shared_double2048, double2048, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedDouble4096, shared_double4096, double4096, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedDouble8192, shared_double8192, double8192, SHARED_FIXED_ARRAY_NUM_SLOTS);

SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat64, shared_float64, float64, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat128, shared_float128, float128, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat256, shared_float256, float256, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat512, shared_float512, float512, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat1024, shared_float1024, float1024, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat2048, shared_float2048, float2048, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat4096, shared_float4096, float4096, SHARED_FIXED_ARRAY_NUM_SLOTS);
SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat8192, shared_float8192, float8192, SHARED_FIXED_ARRAY_NUM_SLOTS);

typedef union shared_double_max_8192 {
    SharedDouble64 sd64;
    SharedDouble128 sd128;
    SharedDouble256 sd256;
    SharedDouble512 sd512 ;
    SharedDouble1024 sd1024 ;
    SharedDouble2048 sd2048 ;
    SharedDouble4096 sd4096 ;
    SharedDouble8192 sd8192 ;
} SharedDoubleMax8192;

typedef union shared_float_max_8192 {
    SharedFloat64 sf64;
    SharedFloat128 sf128;
    SharedFloat256 sf256;
    SharedFloat512 sf512 ;
    SharedFloat1024 sf1024 ;
    SharedFloat2048 sf2048 ;
    SharedFloat4096 sf4096 ;
    SharedFloat8192 sf8192 ;
} SharedFloatMax8192;


#define shared_double_arr_init(shared_double_obj, len)          \
    switch (len) {                                              \
    case 64:                                                    \
        shared_double64_init(&shared_double_obj.sd64);          \
        break;                                                  \
    case 128:                                                   \
        shared_double128_init(&shared_double_obj.sd128);        \
        break;                                                  \
    case 256:                                                   \
        shared_double256_init(&shared_double_obj.sd256);        \
        break;                                                  \
    case 512:                                                   \
        shared_double512_init(&shared_double_obj.sd512);        \
        break;                                                  \
    case 1024:                                                  \
        shared_double1024_init(&shared_double_obj.sd1024);      \
        break;                                                  \
    case 2048:                                                  \
        shared_double2048_init(&shared_double_obj.sd2048);      \
        break;                                                  \
    case 4096:                                                  \
        shared_double4096_init(&shared_double_obj.sd4096);      \
        break;                                                  \
    case 8192:                                                  \
        shared_double8192_init(&shared_double_obj.sd8192);      \
        break;                                                  \
    }


#define shared_double_arr_write(shared_double_ptr, src, len)    \
    switch (len) {                                          \
    case 64: {                                              \
        double64 d;                                         \
        memcpy(d.arr, src, len * sizeof(double));           \
        shared_double64_write(&(shared_double_ptr)->sd64, d);               \
    }                                                       \
        break;                                              \
    case 128: {                                             \
        double128 d;                                        \
        memcpy(d.arr, src, len * sizeof(double));           \
        shared_double128_write(&(shared_double_ptr)->sd128, d);             \
    }                                                       \
        break;                                              \
    case 256: {                                             \
        double256 d;                                        \
        memcpy(d.arr, src, len * sizeof(double));           \
        shared_double256_write(&(shared_double_ptr)->sd256, d);             \
    }                                                       \
        break;                                              \
    case 512: {                                             \
        double512 d;                                        \
        memcpy(d.arr, src, len * sizeof(double));           \
        shared_double512_write(&(shared_double_ptr)->sd512, d);             \
    }                                                       \
        break;                                              \
    case 1024: {                                            \
        double1024 d;                                       \
        memcpy(d.arr, src, len * sizeof(double));           \
        shared_double1024_write(&(shared_double_ptr)->sd1024, d);           \
    }                                                       \
        break;                                              \
    case 2048: {                                            \
        double2048 d;                                       \
        memcpy(d.arr, src, len * sizeof(double));           \
        shared_double2048_write(&(shared_double_ptr)->sd2048, d);           \
    }                                                       \
        break;                                              \
    case 4096: {                                            \
        double4096 d;                                       \
        memcpy(d.arr, src, len * sizeof(double));           \
        shared_double4096_write(&(shared_double_ptr)->sd4096, d);           \
    }                                                       \
        break;                                              \
    case 8192: {                                            \
        double8192 d;                                       \
        memcpy(d.arr, src, len * sizeof(double));           \
        shared_double8192_write(&(shared_double_ptr)->sd8192, d);           \
    }                                                       \
        break;                                              \
    }

#endif
