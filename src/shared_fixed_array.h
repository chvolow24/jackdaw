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

#endif
