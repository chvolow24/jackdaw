#ifndef JDAW_GUI_LT_PARAMS_H
#define JDAW_GUI_LT_PARAMS_H

#include "layout.h"
#include "text.h"

typedef struct lt_params {

    Text *name_label;
    Text *x_type_label;
    Text *y_type_label;
    Text *w_type_label;
    Text *h_type_label;

    Text *name_value;
    Text *x_type_value;
    Text *y_type_value;
    Text *w_type_value;
    Text *h_type_value;

    char *x_value_str;
    char *y_value_str;
    char *w_value_str;
    char *h_value_str;

    Text *x_value;
    Text *y_value;
    Text *w_value;
    Text *h_value;

} LTParams;

void edit_lt_loop(Layout *lt);
void set_lt_params(Layout *lt);

#endif