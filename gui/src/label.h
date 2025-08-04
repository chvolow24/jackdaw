#ifndef JDAW_LABEL_H
#define JDAW_LABEL_H

#include "animation.h"
#include "textbox.h"
#include "value.h"

#define LABEL_COUNTDOWN_MAX 80
#define LABEL_DEFAULT_MAXLEN 24
#define LABEL_H_PAD 4
#define LABEL_V_PAD 2

/* typedef void (*LabelStrFn)(char *dst, size_t dstsize, void *target, ValType type); */
typedef void (*LabelStrFn)(char *dst, size_t dstsize, Value val, ValType type);

typedef struct label {
    /* Layout *layout; */
    char *str;
    int max_len;
    Textbox *tb;
    bool animation_running;
    int countdown_timer;
    int countdown_max;
    ValType val_type;
    LabelStrFn set_str_fn;
    void *target_obj;
    Layout *parent_obj_lt;
    Animation *animation;
} Label;

Label *label_create(
    int max_len,
    Layout *parent_lt,
    LabelStrFn set_str_fn,
    void *target_obj,
    ValType t,
    Window *win);

void label_move(Label *label, int x, int y);
void label_reset(Label *label, Value v);
/* void label_reset(Label *label); */
void label_draw(Label *label);
void label_destroy(Label *label);

void label_amp_to_dbstr(char *dst, size_t dstsize, Value val, ValType t);
void label_pan(char *dst, size_t dstsize, Value val, ValType t);
/* void label_amp_to_dbstr(char *dst, size_t dstsize, float amp); */
/* void label_pan(char *dst, size_t dstsize, float pan); */
void label_msec(char *dst, size_t dstsize, Value v, ValType t);
void label_int_plus_one(char *dst, size_t dstsize, Value v, ValType t);
void label_freq_raw_to_hz(char *dst, size_t dstsize, Value v, ValType t);
/* void label_freq_raw_to_hz(char *dst, size_t dstsize, double raw); */
/* void label_time_samples_to_msec(char *dst, size_t dstsize, int32_t samples, int32_t sample_rate); */
#endif
