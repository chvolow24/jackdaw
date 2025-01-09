#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"

/* Set the value pointed to by vp based on the current value stored at valptr */

void jdaw_val_set(Value *to_set, ValType vt, void *from_ptr)
{
    switch (vt) {
    case JDAW_FLOAT:
	(*to_set).float_v = *((float *)from_ptr);
	break;
    case JDAW_DOUBLE:
	(*to_set).double_v = *((double *)from_ptr);
	break;
    case JDAW_INT:
	(*to_set).int_v = *((int *)from_ptr);
	break;
    case JDAW_UINT8:
	(*to_set).uint8_v = *((uint8_t *)from_ptr);
	break;
    case JDAW_UINT16:
	(*to_set).uint16_v = *((uint16_t *)from_ptr);
	break;
    case JDAW_UINT32:
	(*to_set).uint32_v = *((uint32_t *)from_ptr);
	break;
    case JDAW_INT8:
	(*to_set).int8_v = *((int8_t *)from_ptr);
	break;
    case JDAW_INT16:
	(*to_set).int16_v = *((int16_t *)from_ptr);
	break;
    case JDAW_INT32:
	(*to_set).int32_v = *((int32_t *)from_ptr);
	break;
    case JDAW_BOOL:
	(*to_set).bool_v = *((bool *)from_ptr);
	break;
    default:
	break;
    }
}

void jdaw_val_set_ptr(void *valptr, ValType vt, Value new_value)
{
     switch (vt) {
    case JDAW_FLOAT:
	*((float *)valptr) = new_value.float_v;
	break;
    case JDAW_DOUBLE:
	*((double *)valptr) = new_value.double_v;
	break;
    case JDAW_INT:
	*((int *)valptr) = new_value.int_v;
	break;
    case JDAW_UINT8:
	*((uint8_t *)valptr) = new_value.uint8_v;
	break;
    case JDAW_UINT16:
	*((uint16_t *)valptr) = new_value.uint16_v;
	break;
    case JDAW_UINT32:
	*((uint32_t *)valptr) = new_value.uint32_v;
	break;
    case JDAW_INT8:
	*((int8_t *)valptr) = new_value.int8_v;
	break;
    case JDAW_INT16:
        *((int16_t *)valptr) = new_value.int16_v;
	break;
    case JDAW_INT32:
        *((int32_t *)valptr) = new_value.int32_v;
	break;
    case JDAW_BOOL:
	*((bool *)valptr) = new_value.bool_v;
	break;
    default:
	break;
    }
   
}

Value jdaw_val_from_ptr(void *valptr, ValType vt)
{
    Value ret;
    switch (vt) {
    case JDAW_FLOAT:
	ret.float_v = *((float *)valptr);
	break;
    case JDAW_DOUBLE:
	ret.double_v = *((double *)valptr);
	break;
    case JDAW_INT:
	ret.int_v = *((int *)valptr);
	break;
    case JDAW_UINT8:
	ret.uint8_v = *((uint8_t *)valptr);
	break;
    case JDAW_UINT16:
	ret.uint16_v = *((uint16_t *)valptr);
	break;
    case JDAW_UINT32:
	ret.uint32_v = *((uint32_t *)valptr);
	break;
    case JDAW_INT8:
	ret.int8_v = *((int8_t *)valptr);
	break;
    case JDAW_INT16:
	ret.int16_v = *((int16_t *)valptr);
	break;
    case JDAW_INT32:
	ret.int32_v = *((int32_t *)valptr);
	break;
    case JDAW_BOOL:
	ret.bool_v = *((bool *)valptr);
	break;
    default:
	break;
    }
    return ret;
}

void jdaw_val_set_min(Value *vp, ValType vt)
{
    switch (vt) {
    case JDAW_FLOAT:
	(*vp).float_v = 0.0f;
	break;
    case JDAW_DOUBLE:
	(*vp).double_v = 0.0f;
	break;
    case JDAW_INT:
	(*vp).int_v = INT_MIN;
	break;
    case JDAW_UINT8:
	(*vp).uint8_v = 0;
	break;
    case JDAW_UINT16:
	(*vp).uint16_v = 0;
	break;
    case JDAW_UINT32:
	(*vp).uint32_v = 0;
	break;
    case JDAW_INT8:
	(*vp).int8_v  = INT8_MIN;
	break;
    case JDAW_INT16:
	(*vp).int16_v = INT16_MIN;
	break;
    case JDAW_INT32:
	(*vp).int32_v = INT32_MIN;
	break;
    case JDAW_BOOL:
	(*vp).bool_v = false;
	break;
    default:
	break;
    }
}


void jdaw_val_set_max(Value *vp, ValType vt)
{
    switch (vt) {
    case JDAW_FLOAT:
	(*vp).float_v = 1.0f;
	break;
    case JDAW_DOUBLE:
	(*vp).double_v = 1.0f;
	break;
    case JDAW_INT:
	(*vp).int_v = INT_MAX;
	break;
    case JDAW_UINT8:
	(*vp).uint8_v = UINT8_MAX;
	break;
    case JDAW_UINT16:
	(*vp).uint16_v = UINT16_MAX;
	break;
    case JDAW_UINT32:
	(*vp).uint32_v = UINT32_MAX;
	break;
    case JDAW_INT8:
	(*vp).int8_v  = INT8_MAX;
	break;
    case JDAW_INT16:
	(*vp).int16_v = INT16_MAX;
	break;
    case JDAW_INT32:
	(*vp).int32_v = INT32_MAX;
	break;
    case JDAW_BOOL:
	(*vp).bool_v = true;
	break;
    default:
	break;
    }
}

Value jdaw_val_add(Value a, Value b, ValType vt)
{
    Value ret;
    switch (vt) {
    case JDAW_FLOAT:
        ret.float_v = a.float_v + b.float_v;
	break;
    case JDAW_DOUBLE:
        ret.double_v = a.double_v + b.double_v;
	break;
    case JDAW_INT:
	ret.int_v = a.int_v + b.int_v;
	break;
    case JDAW_UINT8:
	ret.uint8_v = a.uint8_v + b.uint8_v;
	break;
    case JDAW_UINT16:
	ret.uint16_v = a.uint16_v + b.uint16_v;
	break;
    case JDAW_UINT32:
	ret.uint32_v = a.uint32_v + b.uint32_v;
	break;
    case JDAW_INT8:
	ret.int8_v = a.int8_v + b.int8_v;
	break;
    case JDAW_INT16:
	ret.int16_v = a.int16_v + b.int16_v;
	break;
    case JDAW_INT32:
	ret.int32_v = a.int32_v + b.int32_v;
	break;
    case JDAW_BOOL:
	ret.bool_v = a.bool_v + b.bool_v;
	break;
    default:
	break;
    }
    return ret;
}

Value jdaw_val_sub(Value a, Value b, ValType vt)
{
    Value ret;
    switch (vt) {
    case JDAW_FLOAT:
        ret.float_v = a.float_v - b.float_v;
	break;
    case JDAW_DOUBLE:
        ret.double_v = a.double_v - b.double_v;
	break;
    case JDAW_INT:
	ret.int_v = a.int_v - b.int_v;
	break;
    case JDAW_UINT8:
	ret.uint8_v = a.uint8_v - b.uint8_v;
	break;
    case JDAW_UINT16:
	ret.uint16_v = a.uint16_v - b.uint16_v;
	break;
    case JDAW_UINT32:
	ret.uint32_v = a.uint32_v - b.uint32_v;
	break;
    case JDAW_INT8:
	ret.int8_v = a.int8_v - b.int8_v;
	break;
    case JDAW_INT16:
	ret.int16_v = a.int16_v - b.int16_v;
	break;
    case JDAW_INT32:
	ret.int32_v = a.int32_v - b.int32_v;
	break;
    case JDAW_BOOL:
	ret.bool_v = a.bool_v - b.bool_v;
	break;
    default:
	break;
    }
    return ret;
}

Value jdaw_val_mult(Value a, Value b, ValType vt)
{
    Value ret;
    switch (vt) {
    case JDAW_FLOAT:
        ret.float_v = a.float_v * b.float_v;
	break;
    case JDAW_DOUBLE:
        ret.double_v = a.double_v * b.double_v;
	break;
    case JDAW_INT:
	ret.int_v = a.int_v * b.int_v;
	break;
    case JDAW_UINT8:
	ret.uint8_v = a.uint8_v * b.uint8_v;
	break;
    case JDAW_UINT16:
	ret.uint16_v = a.uint16_v * b.uint16_v;
	break;
    case JDAW_UINT32:
	ret.uint32_v = a.uint32_v * b.uint32_v;
	break;
    case JDAW_INT8:
	ret.int8_v = a.int8_v * b.int8_v;
	break;
    case JDAW_INT16:
	ret.int16_v = a.int16_v * b.int16_v;
	break;
    case JDAW_INT32:
	ret.int32_v = a.int32_v * b.int32_v;
	break;
    case JDAW_BOOL:
	ret.bool_v = a.bool_v * b.bool_v;
	break;
    default:
	break;
    }
    return ret;
}


Value jdaw_val_div(Value a, Value b, ValType vt)
{
    Value ret;
    switch (vt) {
    case JDAW_FLOAT:
        ret.float_v = a.float_v / b.float_v;
	break;
    case JDAW_DOUBLE:
        ret.double_v = a.double_v / b.double_v;
	break;
    case JDAW_INT:
	ret.int_v = a.int_v / b.int_v;
	break;
    case JDAW_UINT8:
	ret.uint8_v = a.uint8_v / b.uint8_v;
	break;
    case JDAW_UINT16:
	ret.uint16_v = a.uint16_v / b.uint16_v;
	break;
    case JDAW_UINT32:
	ret.uint32_v = a.uint32_v / b.uint32_v;
	break;
    case JDAW_INT8:
	ret.int8_v = a.int8_v / b.int8_v;
	break;
    case JDAW_INT16:
	ret.int16_v = a.int16_v / b.int16_v;
	break;
    case JDAW_INT32:
	ret.int32_v = a.int32_v / b.int32_v;
	break;
    case JDAW_BOOL:
	ret.bool_v = a.bool_v / b.bool_v;
	break;
    default:
	break;
    }
    return ret;
}

Value jdaw_val_scale(Value a, double scalar, ValType vt)
{
        Value ret;
    switch (vt) {
    case JDAW_FLOAT:
        ret.float_v = a.float_v * scalar;
	break;
    case JDAW_DOUBLE:
        ret.double_v = a.double_v * scalar;
	break;
    case JDAW_INT:
	ret.int_v = (int)((double)a.int_v * scalar);
	break;
    case JDAW_UINT8:
	ret.uint8_v = (uint8_t)((double)a.uint8_v * scalar);
	break;
    case JDAW_UINT16:
	ret.uint16_v = (uint16_t)((double)a.uint16_v * scalar);
	break;
    case JDAW_UINT32:
	ret.uint32_v = (uint32_t)((double)a.uint32_v * scalar);
	break;
    case JDAW_INT8:
	ret.int8_v = (int8_t)((double)a.int8_v * scalar);
	break;
    case JDAW_INT16:
	ret.int16_v = (int16_t)((double)a.int16_v * scalar);
	break;
    case JDAW_INT32:
	ret.int32_v = (int32_t)((double)a.int32_v * scalar);
	break;
    case JDAW_BOOL:
	ret.bool_v = a.bool_v * scalar;
	break;
    default:
	break;
    }
    return ret;
}

double jdaw_val_div_double(Value a, Value b, ValType vt)
{
    double ret;
    switch (vt) {
    case JDAW_FLOAT:
        ret = (double)a.float_v / b.float_v;
	break;
    case JDAW_DOUBLE:
        ret = (double)a.double_v / b.double_v;
	break;
    case JDAW_INT:
	ret = (double)a.int_v / b.int_v;
	break;
    case JDAW_UINT8:
	ret = (double)a.uint8_v / b.uint8_v;
	break;
    case JDAW_UINT16:
	ret = (double)a.uint16_v / b.uint16_v;
	break;
    case JDAW_UINT32:
	ret = (double)a.uint32_v / b.uint32_v;
	break;
    case JDAW_INT8:
	ret = (double)a.int8_v / b.int8_v;
	break;
    case JDAW_INT16:
	ret = (double)a.int16_v / b.int16_v;
	break;
    case JDAW_INT32:
	ret = (double)a.int32_v / b.int32_v;
	break;
    case JDAW_BOOL:
	ret = (double)a.bool_v / b.bool_v;
	break;
    default:
	break;
    }
    return ret;
}

void jdaw_val_set_str(char *dst, size_t dstsize, Value v, ValType type, int decimal_places)
{

    switch (type) {
    case JDAW_FLOAT:
	snprintf(dst, dstsize, "%.*f", decimal_places, v.float_v);
	break;
    case JDAW_DOUBLE:
	snprintf(dst, dstsize, "%.*f", decimal_places, v.double_v);
	break;
    case JDAW_INT:
	snprintf(dst, dstsize, "%d", v.int_v);
	break;
    case JDAW_UINT8:
	snprintf(dst, dstsize, "%du", v.uint8_v);
	break;
    case JDAW_UINT16:
	snprintf(dst, dstsize, "%du", v.uint16_v);
	break;
    case JDAW_UINT32:
	snprintf(dst, dstsize, "%du", v.uint32_v);
	break;
    case JDAW_INT8:
	snprintf(dst, dstsize, "%d", v.int8_v);
	break;
    case JDAW_INT16:
	snprintf(dst, dstsize, "%d", v.int16_v);
	break;
    case JDAW_INT32:
	snprintf(dst, dstsize, "%d", v.int32_v);
	break;
    case JDAW_BOOL:
	snprintf(dst, dstsize, v.bool_v ? "true" : "false");
	break;
    default:
	break;
    }
    dst[dstsize - 1] = '\0';
}


void jdaw_valptr_set_str(char *dst, size_t dstsize, void *value, ValType type, int decimal_places)
{

    Value v = jdaw_val_from_ptr(value, type);
    switch (type) {
    case JDAW_FLOAT:
	snprintf(dst, dstsize, "%.*f", decimal_places, v.float_v);
	break;
    case JDAW_DOUBLE:
	snprintf(dst, dstsize, "%.*f", decimal_places, v.double_v);
	break;
    case JDAW_INT:
	snprintf(dst, dstsize, "%d", v.int_v);
	break;
    case JDAW_UINT8:
	snprintf(dst, dstsize, "%du", v.uint8_v);
	break;
    case JDAW_UINT16:
	snprintf(dst, dstsize, "%du", v.uint16_v);
	break;
    case JDAW_UINT32:
	snprintf(dst, dstsize, "%du", v.uint32_v);
	break;
    case JDAW_INT8:
	snprintf(dst, dstsize, "%d", v.int8_v);
	break;
    case JDAW_INT16:
	snprintf(dst, dstsize, "%d", v.int16_v);
	break;
    case JDAW_INT32:
	snprintf(dst, dstsize, "%d", v.int32_v);
	break;
    case JDAW_BOOL:
	snprintf(dst, dstsize, v.bool_v ? "true" : "false");
	break;
    default:
	break;
    }
    dst[dstsize - 1] = '\0';
}


bool jdaw_val_less_than(Value a, Value b, ValType type)
{
    switch (type) {
    case JDAW_FLOAT:
	return a.float_v < b.float_v;
    case JDAW_DOUBLE:
	return a.double_v < b.double_v;
    case JDAW_INT:
	return a.int_v < b.int_v;
    case JDAW_UINT8:
	return a.uint8_v < b.uint8_v;
    case JDAW_UINT16:
	return a.uint16_v < b.uint16_v;
    case JDAW_UINT32:
	return a.uint32_v < b.uint32_v;
    case JDAW_INT8:
	return a.int8_v < b.int8_v;
    case JDAW_INT16:
	return a.int16_v < b.int16_v;
    case JDAW_INT32:
	return a.int32_v < b.int32_v;
    case JDAW_BOOL:
	return a.bool_v < b.bool_v;
    default:
	return 0;
	break;
    }
}

bool jdaw_val_is_zero(Value a, ValType type)
{
    double epsilon = 1e-9;
    switch (type) {
    case JDAW_FLOAT:
	return fabs(a.float_v) < epsilon;
    case JDAW_DOUBLE:
	return fabs(a.double_v) < epsilon;
    case JDAW_INT:
	return a.int_v == 0;
    case JDAW_UINT8:
	return a.uint8_v == 0;
    case JDAW_UINT16:
	return a.uint16_v == 0;
    case JDAW_UINT32:
	return a.uint32_v == 0;
    case JDAW_INT8:
	return a.int8_v == 0;
    case JDAW_INT16:
	return a.int16_v == 0;
    case JDAW_INT32:
	return a.int32_v == 0;
    case JDAW_BOOL:
	return a.bool_v == false;
    default:
	return 0;
	break;
    }
}

bool jdaw_val_equal(Value a, Value b, ValType type)
{
    double epsilon = 1e-9;
    switch (type) {
    case JDAW_FLOAT:
	return fabs(fabs(a.float_v) - fabs(b.float_v)) < epsilon;
    case JDAW_DOUBLE:
	return fabs(fabs(a.double_v) - fabs(b.float_v)) < epsilon;
    case JDAW_INT:
	return a.int_v == b.int_v;
    case JDAW_UINT8:
	return a.uint8_v == b.uint8_v;
    case JDAW_UINT16:
	return a.uint16_v == b.uint16_v;
    case JDAW_UINT32:
	return a.uint32_v == b.uint32_v;
    case JDAW_INT8:
	return a.int8_v == b.int8_v;
    case JDAW_INT16:
	return a.int16_v == b.int16_v;
    case JDAW_INT32:
	return a.int32_v == b.int32_v;
    case JDAW_BOOL:
	return a.bool_v == b.bool_v;
    default:
	return 0;
	break;
    }
}


bool jdaw_val_sign_match(Value a, Value b, ValType type)
{
    switch (type) {
    case JDAW_FLOAT:
	return a.float_v * b.float_v >= 0.0f;
    case JDAW_DOUBLE:
	return a.double_v * b.double_v >= 0.0;
    case JDAW_INT:
	return a.int_v * b.int_v > 0;
    case JDAW_UINT8:
    case JDAW_UINT16:
    case JDAW_UINT32:
	return true;
    case JDAW_INT8:
	return a.int8_v * b.int8_v > 0;
    case JDAW_INT16:
	return a.int16_v * b.int16_v > 0;
    case JDAW_INT32:
	return a.int32_v * b.int32_v > 0;
    case JDAW_BOOL:
	return a.bool_v = b.bool_v;
    default:
	return 0;
	break;
    }
}

void jdaw_val_serialize(Value v, ValType type, FILE *f, uint8_t dstsize)
{

    char dst[dstsize + 1];
    switch (type) {
    case JDAW_FLOAT:
	snprintf(dst, dstsize, "%.*g", dstsize, v.float_v);
	break;
    case JDAW_DOUBLE:
	snprintf(dst, dstsize, "%.*g", dstsize, v.double_v);
	break;
    case JDAW_INT:
	snprintf(dst, dstsize, "%d", v.int_v);
	break;
    case JDAW_UINT8:
	snprintf(dst, dstsize, "%d", v.uint8_v);
	break;
    case JDAW_UINT16:
	snprintf(dst, dstsize, "%d", v.uint16_v);
	break;	
    case JDAW_UINT32:
	snprintf(dst, dstsize, "%d", v.uint32_v);
	break;
    case JDAW_INT8:
	snprintf(dst, dstsize, "%d", v.int8_v);
	break;
    case JDAW_INT16:
	snprintf(dst, dstsize, "%d", v.int16_v);
	break;
    case JDAW_INT32:
	snprintf(dst, dstsize, "%d", v.int32_v);
	break;
    case JDAW_BOOL:
	snprintf(dst, dstsize, "%d", v.bool_v);
	break;
    default:
	break;
    }
    dst[dstsize] = '\0';
    fwrite(dst, 1, dstsize, f);
}




Value jdaw_val_deserialize(FILE *f, uint8_t size, ValType type)
{
    char buf[size + 1];
    fread(buf, 1, size, f);
    buf[size] = '\0';
    
    Value ret;
    switch (type) {
    case JDAW_FLOAT:
	ret.float_v = atof(buf);
	break;
    case JDAW_DOUBLE:
	ret.double_v = atof(buf);
	break;
    case JDAW_INT:
	ret.int_v = atoi(buf);
	break;
    case JDAW_UINT8:
	ret.uint8_v = atoi(buf);
	break;
    case JDAW_UINT16:
	ret.uint16_v = atoi(buf);
	break;
    case JDAW_UINT32:
	ret.uint32_v = atoi(buf);
	break;
    case JDAW_INT8:
	ret.int8_v = atoi(buf);
	break;
    case JDAW_INT16:
	ret.int16_v = atoi(buf);
	break;
    case JDAW_INT32:
	ret.int32_v = atoi(buf);
	break;
    case JDAW_BOOL:
	ret.bool_v = atoi(buf);
	break;
    default:
	break;
    }
    return ret;
}
