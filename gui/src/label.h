#ifndef JDAW_LABEL_H
#define JDAW_LABEL_H

#include "textbox.h"
#include "value.h"

#define LABEL_COUNTDOWN_MAX 80
#define LABEL_DEFAULT_MAXLEN 24
#define LABEL_H_PAD 8
#define LABEL_V_PAD 2

typedef void (*LabelStrFn)(char *dst, size_t dstsize, ValType t, void *target);

typedef struct label {
    /* Layout *layout; */
    char *str;
    int max_len;
    Textbox *tb;
    int countdown_timer;
    int countdown_max;
    ValType val_type;
    LabelStrFn set_str_fn;
    void *target_obj;
} Label;

Label *label_create(
    int max_len,
    Layout *parent_lt,
    LabelStrFn set_str_fn,
    void *target_obj,
    ValType t,
    Window *win);

void label_reset(Label *label);
void label_draw(Label *label);
void label_destroy(Label *label);
#endif
