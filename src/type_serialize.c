#include "type_serialize.h"
uint16_t uint16_fromstr_le(char *buf)
{
    uint16_t val = 0;
    uint8_t *ubuf = (uint8_t *)buf;
    val += (uint16_t)ubuf[0];
    val += (uint16_t)ubuf[1]<<8;
    return val;
}

uint32_t uint32_fromstr_le(char *buf)
{
    uint32_t val = 0;
    uint8_t *ubuf = (uint8_t *)buf;
    val += (uint32_t)ubuf[0];
    val += (uint32_t)ubuf[1]<<8;
    val += (uint32_t)ubuf[2]<<16;
    val += (uint32_t)ubuf[3]<<24;
    return val;
}

uint64_t uint64_fromstr_le(char *buf)
{
    uint64_t val = 0;
    uint8_t *ubuf = (uint8_t *)buf;
    val += (uint64_t)ubuf[0];
    val += (uint64_t)ubuf[1]<<8;
    val += (uint64_t)ubuf[2]<<16;
    val += (uint64_t)ubuf[3]<<24;
    val += (uint64_t)ubuf[4]<<32;
    val += (uint64_t)ubuf[5]<<40;
    val += (uint64_t)ubuf[6]<<48;
    val += (uint64_t)ubuf[7]<<56;
    return val;
}

void uint16_tostr_le(uint16_t val, char *buf)
{
    buf[1] = (val >> 8) & 0xFF;
    buf[0] = val & 0xFF;
}

void uint32_tostr_le(uint32_t val, char *buf)
{
    buf[3] = (val>>24) & 0xff;
    buf[2] = (val>>16) & 0xff;
    buf[1] = (val>>8) & 0xff;
    buf[0] = val & 0xff;
}

void uint64_tostr_le(uint64_t val, char *buf)
{
    buf[7] = (val >> 56) & 0xFF;
    buf[6] = (val >> 48) & 0xFF;
    buf[5] = (val >> 40) & 0xFF;
    buf[4] = (val >> 32) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[0] = val & 0xFF;
}

/* SIZE 40 (5 bytes) */
void float_tostr40_le(double ts, char *buf)
{
    int exp;
    double mantissa = frexp(ts, &exp);
    int32_t mant_sint = (int32_t)(mantissa * INT32_MAX);
    int8_t exp_sint = (int8_t)exp;
    uint8_t exp_uint = *((uint8_t *)&exp_sint);
    uint32_t mant_uint = *((uint32_t *)&mant_sint);
    buf[0] = exp_uint;
    uint32_tostr_le(mant_uint, buf + 1);
}

double float_fromstr40_le(char *buf)
{
    uint8_t exp_uint = buf[0];
    uint32_t mant_uint = uint32_fromstr_le(buf + 1);
    int32_t mant_sint = *((int32_t *)&mant_uint);
    int8_t exp_sint = *((int8_t *)&exp_uint);
    int exp = (int)exp_sint;
    double mantissa = (double)mant_sint / INT32_MAX;
    return ldexp(mantissa, exp);
}

void uint8_ser(FILE *f, uint8_t *val_p)
{
    fwrite(val_p, 1, 1, f);
}

void uint16_ser_le(FILE *f, uint16_t *val_p)
{
    uint16_t val = *val_p;
    char buf[2];
    uint16_tostr_le(val, buf);
    fwrite(buf, 1, 2, f);
}

void uint32_ser_le(FILE *f, uint32_t *val_p)
{
    uint32_t val = *val_p;
    char buf[4];
    uint32_tostr_le(val, buf);
    fwrite(buf, 1, 4, f);
}

void uint64_ser_le(FILE *f, uint64_t *val_p)
{
    uint64_t val = *val_p;
    char buf[8];
    uint64_tostr_le(val, buf);
    fwrite(buf, 1, 8, f);
}

void float_ser40_le(FILE *f, double val)
{
    char buf[5];
    float_tostr40_le(val, buf);
    fwrite(buf, 1, 5, f);
}

uint8_t uint8_deser(FILE *f)
{
    uint8_t ret;
    fread(&ret, 1, 1, f);
    return ret;
}

uint16_t uint16_deser_le(FILE *f)
{
    char buf[2];
    fread(buf, 1, 2, f);
    return uint16_fromstr_le(buf);
}

uint32_t uint32_deser_le(FILE *f)
{
    char buf[4];
    fread(buf, 1, 4, f);
    return uint32_fromstr_le(buf);
}

uint64_t uint64_deser_le(FILE *f)
{
    char buf[8];
    fread(buf, 1, 8, f);
    return uint64_fromstr_le(buf);
}

double float_deser40_le(FILE *f)
{
    char buf[5];
    fread(buf, 1, 5, f);
    return float_fromstr40_le(buf);
}

void int8_ser(FILE *f, int8_t *v_p)
{
    uint8_ser(f, (uint8_t *)v_p);
}

void int16_ser_le(FILE *f, int16_t *v_p)
{
    uint16_ser_le(f, (uint16_t *)v_p);
}

void int32_ser_le(FILE *f, int32_t *v_p)
{
    uint32_ser_le(f, (uint32_t *)v_p);
}

void int64_ser_le(FILE *f, int64_t *v_p)
{
    uint64_ser_le(f, (uint64_t *)v_p);
}

void int_ser_le(FILE *f, int *v_p)
{
    int32_t v = *v_p;
    int32_ser_le(f, &v);
}

int8_t int8_deser(FILE *f)
{
    int8_t ret;
    fread(&ret, 1, 1, f);
    return ret;
}

int16_t int16_deser_le(FILE *f)
{
    uint16_t uv = uint16_deser_le(f);
    return *(int16_t *)&uv;
}

int32_t int32_deser_le(FILE *f)
{
    uint32_t uv = uint32_deser_le(f);
    return *(int32_t *)&uv;

}

int64_t int64_deser_le(FILE *f)
{
    uint64_t uv = uint64_deser_le(f);
    return *(int64_t *)&uv;

}

int int_deser_le(FILE *f)
{
    uint32_t uv = uint32_deser_le(f);
    int32_t v = *((int32_t *)&uv);
    return (int)v;
}
