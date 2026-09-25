#ifndef JDAW_SHARED_VALUE_DECL
#define JDAW_SHARED_VALUE_DECL

#include "shared_value.h"
#include "value.h"

SHARED_VALUE_TYPE_DECL_ONLY(SharedValue, shared_value, Value, 4);
SHARED_VALUE_TYPE_DECL_ONLY(SharedFloat, shared_float, float, 4);

#endif
