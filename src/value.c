#include "type_serialize.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"

/* Set the value pointed to by vp based on the current value stored at valptr */

void jdaw_val_set(Value *vp, ValType vt, void *valptr)
{
    switch (vt) {
    case JDAW_FLOAT:
	(*vp).float_v = *((float *)valptr);
	break;
    case JDAW_DOUBLE:
	(*vp).double_v = *((double *)valptr);
	break;
    case JDAW_INT:
	(*vp).int_v = *((int *)valptr);
	break;
    case JDAW_UINT8:
	(*vp).uint8_v = *((uint8_t *)valptr);
	break;
    case JDAW_UINT16:
	(*vp).uint16_v = *((uint16_t *)valptr);
	break;
    case JDAW_UINT32:
	(*vp).uint32_v = *((uint32_t *)valptr);
	break;
    case JDAW_INT8:
	(*vp).int8_v = *((int8_t *)valptr);
	break;
    case JDAW_INT16:
	(*vp).int16_v = *((int16_t *)valptr);
	break;
    case JDAW_INT32:
	(*vp).int32_v = *((int32_t *)valptr);
	break;
    case JDAW_BOOL:
	(*vp).bool_v = *((bool *)valptr);
	break;
    case JDAW_DOUBLE_PAIR: {
	double *pair = (double *)valptr;
	(*vp).double_pair_v[0] = pair[0];
	(*vp).double_pair_v[1] = pair[1];
    }
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
     case JDAW_DOUBLE_PAIR:
	 *((double *)valptr) = new_value.double_pair_v[0];
	 *((double *)valptr + 1) = new_value.double_pair_v[1];
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
    case JDAW_DOUBLE_PAIR:
	ret.double_pair_v[0] = *((double *)valptr);
	ret.double_pair_v[1] = *((double *)valptr + 1);
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
    case JDAW_DOUBLE_PAIR:
	(*vp).double_pair_v[0] = 0.0;
	(*vp).double_pair_v[1] = 1.0;
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
    case JDAW_DOUBLE_PAIR:
	(*vp).double_pair_v[0] = 1.0;
	(*vp).double_pair_v[1] = 1.0;
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
    case JDAW_DOUBLE_PAIR:
	ret.double_pair_v[0] = a.double_pair_v[0] + b.double_pair_v[0];
	ret.double_pair_v[1] = a.double_pair_v[1] + b.double_pair_v[1];
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
    case JDAW_DOUBLE_PAIR:
	ret.double_pair_v[0] = a.double_pair_v[0] - b.double_pair_v[0];
	ret.double_pair_v[1] = a.double_pair_v[1] - b.double_pair_v[1];
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
    case JDAW_DOUBLE_PAIR:
	ret.double_pair_v[0] = a.double_pair_v[0] * b.double_pair_v[0];
	ret.double_pair_v[1] = a.double_pair_v[1] * b.double_pair_v[1];
	break;

    default:
	break;
    }
    return ret;
}

Value jdaw_val_negate(Value a, ValType vt)
{
    switch (vt) {
    case JDAW_FLOAT:
	a.float_v *= -1;
	break;
    case JDAW_DOUBLE:
	a.double_v *= -1;
	break;
    case JDAW_INT:
	a.int_v *= -1;
	break;
    case JDAW_UINT8:
	a.uint8_v *= -1;
	break;
    case JDAW_UINT16:
	a.uint16_v *= -1;
	break;
    case JDAW_UINT32:
	a.uint32_v *= -1;
	break;
    case JDAW_INT8:
	a.int8_v *= -1;
	break;
    case JDAW_INT16:
	a.int16_v *= -1;
	break;
    case JDAW_INT32:
	a.int32_v *= -1;
	break;
    case JDAW_BOOL:
	a.bool_v = !a.bool_v;
	break;
    case JDAW_DOUBLE_PAIR:
	a.double_pair_v[0] = a.double_pair_v[0] * -1;
	a.double_pair_v[1] = a.double_pair_v[1] * -1;
	break;
    default:
	break;

    }
    return a;    
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
    case JDAW_DOUBLE_PAIR:
	ret.double_pair_v[0] = a.double_pair_v[0] / b.double_pair_v[0];
	ret.double_pair_v[1] = a.double_pair_v[1] / b.double_pair_v[1];
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
    case JDAW_BOOL: {
	if (scalar > -0.5 && scalar < 0.5)
	    ret.bool_v = false;
	else 
	    ret.bool_v = true;
    }
	break;
    case JDAW_DOUBLE_PAIR:
	ret.double_pair_v[0] = a.double_pair_v[0] * scalar;
	ret.double_pair_v[1] = a.double_pair_v[1] * scalar;
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
    case JDAW_DOUBLE_PAIR:
	ret = a.double_pair_v[0] / b.double_pair_v[0];
	/* ret.double_pair_v[0] = a.double_pair_v[0] * -1; */
	/* ret.double_pair_v[1] = a.double_pair_v[1] * -1; */
	break;

    default:
	break;
    }
    return ret;
}

/* void jdaw_val_set_str(char *dst, size_t dstsize, Value v, ValType type, int decimal_places) */
/* { */

/*     switch (type) { */
/*     case JDAW_FLOAT: */
/* 	snprintf(dst, dstsize, "%.*f", decimal_places, v.float_v); */
/* 	break; */
/*     case JDAW_DOUBLE: */
/* 	snprintf(dst, dstsize, "%.*f", decimal_places, v.double_v); */
/* 	break; */
/*     case JDAW_INT: */
/* 	snprintf(dst, dstsize, "%d", v.int_v); */
/* 	break; */
/*     case JDAW_UINT8: */
/* 	snprintf(dst, dstsize, "%du", v.uint8_v); */
/* 	break; */
/*     case JDAW_UINT16: */
/* 	snprintf(dst, dstsize, "%du", v.uint16_v); */
/* 	break; */
/*     case JDAW_UINT32: */
/* 	snprintf(dst, dstsize, "%du", v.uint32_v); */
/* 	break; */
/*     case JDAW_INT8: */
/* 	snprintf(dst, dstsize, "%d", v.int8_v); */
/* 	break; */
/*     case JDAW_INT16: */
/* 	snprintf(dst, dstsize, "%d", v.int16_v); */
/* 	break; */
/*     case JDAW_INT32: */
/* 	snprintf(dst, dstsize, "%d", v.int32_v); */
/* 	break; */
/*     case JDAW_BOOL: */
/* 	snprintf(dst, dstsize, v.bool_v ? "true" : "false"); */
/* 	break; */
/*     case JDAW_DOUBLE_PAIR: */
/* 	snprintf(dst, dstsize, "[%f,%f]", v.double_pair_v[0], v.double_pair_v[1]); */
/* 	break; */

/*     default: */
/* 	break; */
/*     } */
/*     dst[dstsize - 1] = '\0'; */
/* } */

void jdaw_val_to_str(char *dst, size_t dstsize, Value v, ValType type, int decimal_places);
void jdaw_valptr_to_str(char *dst, size_t dstsize, void *value, ValType type, int decimal_places)
{

    Value v = jdaw_val_from_ptr(value, type);
    jdaw_val_to_str(dst, dstsize, v, type, decimal_places);
    
    /* switch (type) { */
    /* case JDAW_FLOAT: */
    /* 	snprintf(dst, dstsize, "%.*f", decimal_places, v.float_v); */
    /* 	break; */
    /* case JDAW_DOUBLE: */
    /* 	snprintf(dst, dstsize, "%.*f", decimal_places, v.double_v); */
    /* 	break; */
    /* case JDAW_INT: */
    /* 	snprintf(dst, dstsize, "%d", v.int_v); */
    /* 	break; */
    /* case JDAW_UINT8: */
    /* 	snprintf(dst, dstsize, "%du", v.uint8_v); */
    /* 	break; */
    /* case JDAW_UINT16: */
    /* 	snprintf(dst, dstsize, "%du", v.uint16_v); */
    /* 	break; */
    /* case JDAW_UINT32: */
    /* 	snprintf(dst, dstsize, "%du", v.uint32_v); */
    /* 	break; */
    /* case JDAW_INT8: */
    /* 	snprintf(dst, dstsize, "%d", v.int8_v); */
    /* 	break; */
    /* case JDAW_INT16: */
    /* 	snprintf(dst, dstsize, "%d", v.int16_v); */
    /* 	break; */
    /* case JDAW_INT32: */
    /* 	snprintf(dst, dstsize, "%d", v.int32_v); */
    /* 	break; */
    /* case JDAW_BOOL: */
    /* 	snprintf(dst, dstsize, v.bool_v ? "true" : "false"); */
    /* 	break; */
    /* case JDAW_DOUBLE_PAIR: */
    /* 	snprintf(dst, dstsize, "[%f,%f]", v.double_pair_v[0], v.double_pair_v[1]); */
    /* 	break; */
    /* default: */
    /* 	break; */
    /* } */
    /* dst[dstsize - 1] = '\0'; */
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
    case JDAW_DOUBLE_PAIR:
	return a.double_pair_v[0] < b.double_pair_v[0];
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
    case JDAW_DOUBLE_PAIR:
	return
	    fabs(a.double_pair_v[0]) < epsilon &&
	    fabs(a.double_pair_v[1]) < epsilon;
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
	return fabs(a.float_v - b.float_v) < epsilon;
    case JDAW_DOUBLE:
	return fabs(a.double_v - b.double_v) < epsilon;
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
    case JDAW_DOUBLE_PAIR:
        return
	    fabs(a.double_pair_v[0] - b.double_pair_v[1]) < epsilon &&
	    fabs(a.double_pair_v[1] - b.double_pair_v[1]) < epsilon;
	break;
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
    case JDAW_DOUBLE_PAIR:
        return
	    fabs(a.double_pair_v[0] * b.double_pair_v[1]) >= 0.0 &&
	    fabs(a.double_pair_v[1] - b.double_pair_v[1]) >= 0.0;
	break;

    default:
	return 0;
	break;
    }
}

/* Variable size, depending on type */
void jdaw_val_serialize(FILE *f, Value v, ValType type)
{
    uint8_t type_byte = (uint8_t)type;
    fwrite(&type_byte, 1, 1, f);
    switch (type) {
    case JDAW_FLOAT:
	float_ser40_le(f, v.float_v);
	break;
    case JDAW_DOUBLE:
	float_ser40_le(f, v.double_v);
	break;
    case JDAW_INT:
        int_ser_le(f, &v.int_v);
	break;
    case JDAW_UINT8:
	fwrite(&v.uint8_v, 1, 1, f);
	break;
    case JDAW_UINT16:
	uint16_ser_le(f, &v.uint16_v);
	break;	
    case JDAW_UINT32:
	uint32_ser_le(f, &v.uint32_v);
	break;
    case JDAW_INT8:
	fwrite(&v.int8_v, 1, 1, f);
	break;
    case JDAW_INT16:
	int16_ser_le(f, &v.int16_v);
	break;
    case JDAW_INT32:
	int32_ser_le(f, &v.int32_v);
	break;
    case JDAW_BOOL: {
	uint8_t bool_byte = (uint8_t)(v.bool_v);
	fwrite(&bool_byte, 1, 1, f);
    }
	break;
    case JDAW_DOUBLE_PAIR:
	float_ser40_le(f, v.double_pair_v[0]);
	float_ser40_le(f, v.double_pair_v[1]);
	break;
    default:
	break;
    }
}

Value jdaw_val_deserialize(FILE *f)
{
    uint8_t type_byte;
    fread(&type_byte, 1, 1, f);
    ValType type = (ValType)((int)type_byte);
    Value ret = {0};
    switch (type) {
    case JDAW_FLOAT:
	ret.float_v = float_deser40_le(f);
	break;
    case JDAW_DOUBLE:
	ret.double_v = float_deser40_le(f);
	break;
    case JDAW_INT:
        ret.int_v = int_deser_le(f);
	break;
    case JDAW_UINT8:
	ret.uint8_v = fread(&ret.uint8_v, 1, 1, f);
	break;
    case JDAW_UINT16:
	ret.uint16_v = uint16_deser_le(f);
	break;	
    case JDAW_UINT32:
	ret.uint32_v = uint32_deser_le(f);
	break;
    case JDAW_INT8:
	fread(&ret.int8_v, 1, 1, f);
	break;
    case JDAW_INT16:
	ret.int16_v = int16_deser_le(f);
	break;
    case JDAW_INT32:
	ret.int32_v = int32_deser_le(f);
	break;
    case JDAW_BOOL: {
	uint8_t bool_byte;
	fread(&bool_byte, 1, 1, f);
	ret.bool_v = (bool)bool_byte;
    }
    case JDAW_DOUBLE_PAIR:
	ret.double_pair_v[0] = float_deser40_le(f);
	ret.double_pair_v[1] = float_deser40_le(f);
	break;
    default:
	break;
    }
    return ret;
}
void jdaw_val_serialize_OLD(Value v, ValType type, FILE *f, uint8_t dstsize)
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


void jdaw_val_to_str(char *dst, size_t dstsize, Value v, ValType type, int decimal_places)
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
    case JDAW_DOUBLE_PAIR:
	snprintf(dst, dstsize, "[%f,%f]", v.double_pair_v[0], v.double_pair_v[1]);
	break;
    default:
	break;
    }
}

Value jdaw_val_from_str(const char *str, ValType type)
{
    Value ret;
    switch (type) {
    case JDAW_FLOAT:
	ret.float_v = atof(str);
	break;
    case JDAW_DOUBLE:
	ret.double_v = atof(str);
	break;
    case JDAW_INT:
	ret.int_v = atoi(str);
	break;
    case JDAW_UINT8:
	ret.uint8_v = atoi(str);
	break;
    case JDAW_UINT16:
	ret.uint16_v = atoi(str);
	break;
    case JDAW_UINT32:
	ret.uint32_v = atoi(str);
	break;
    case JDAW_INT8:
	ret.int8_v = atoi(str);
	break;
    case JDAW_INT16:
	ret.int16_v = atoi(str);
	break;
    case JDAW_INT32:
	ret.int32_v = atoi(str);
	break;
    case JDAW_BOOL:
	ret.bool_v = atoi(str);
	break;
    case JDAW_DOUBLE_PAIR: {
	char *cpy = strdup(str);
	cpy++; /* first char is [ */
	str += 1;
	char *comma = strchr(str, ',');
	if (!comma) {
	    ret.double_pair_v[0] = 0.0;
	    ret.double_pair_v[1] = 0.0;
	    break;
	}
	*comma = '\0';
	ret.double_pair_v[0] = atof(cpy);
	comma++;
	cpy = strchr(comma, ']');
	if (!cpy) {
	    ret.double_pair_v[0] = 0.0;
	    ret.double_pair_v[1] = 0.0;
	    break;
	}
	*cpy = '\0';
	ret.double_pair_v[1] = atof(comma);	
	free(cpy);
    }
    default:
	break;
    }
    return ret;
}



Value jdaw_val_deserialize_OLD(FILE *f, uint8_t size, ValType type)
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
    case JDAW_DOUBLE_PAIR:
	memset(&ret, '\0', sizeof(ret));
    default:
	break;
    }
    return ret;
}


bool jdaw_val_in_range(Value test, Value min, Value max, ValType type)
{
    switch (type) {
    case JDAW_FLOAT:
	return min.float_v <= test.float_v && test.float_v <= max.float_v;
    case JDAW_DOUBLE:
	return min.double_v <= test.double_v && test.double_v <= max.double_v;
    case JDAW_INT:
	return min.int_v <= test.int_v && test.int_v <= max.double_v;
    case JDAW_UINT8:
	return min.uint8_v <= test.uint8_v && test.uint8_v <= max.uint8_v;
    case JDAW_UINT16:
	return min.uint16_v <= test.uint16_v && test.uint16_v <= max.uint16_v;
    case JDAW_UINT32:
	return min.uint32_v <= test.uint32_v && test.uint32_v <= max.uint32_v;
    case JDAW_INT8:
	return min.int8_v <= test.int8_v && test.int8_v <= max.int8_v;
    case JDAW_INT16:
	return min.int16_v <= test.int16_v && test.int16_v <= max.int16_v;
    case JDAW_INT32:
	return min.int32_v <= test.int32_v && test.int32_v <= max.int32_v;
    case JDAW_BOOL:
	true;
    case JDAW_DOUBLE_PAIR:
	return
	    min.double_pair_v[0] <= test.double_pair_v[0] && test.double_pair_v[0] <= max.double_pair_v[0]
	    && min.double_pair_v[1] <= test.double_pair_v[1] && test.double_pair_v[1] <= max.double_pair_v[1];
    default:
	return 0;
	break;
    }
}

size_t jdaw_val_type_size(ValType type)
{
    switch (type) {
    case JDAW_FLOAT:
	return sizeof(float);
    case JDAW_DOUBLE:
	return sizeof(double);
    case JDAW_INT:
	return sizeof(int);
    case JDAW_UINT8:
	return sizeof(uint8_t);
    case JDAW_UINT16:
	return sizeof(uint16_t);
    case JDAW_UINT32:
	return sizeof(uint32_t);
    case JDAW_INT8:
	return sizeof(int8_t);
    case JDAW_INT16:
	return sizeof(int16_t);
    case JDAW_INT32:
	return sizeof(int32_t);
    case JDAW_BOOL:
	return sizeof(bool);
    case JDAW_DOUBLE_PAIR:
	return 2 * sizeof(double);
    default:
	return 0;
	break;
    }
}
