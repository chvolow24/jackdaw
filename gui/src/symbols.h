#ifndef JDAW_SYMBOLS_H
#define JDAW_SYMBOLS_H

#include "window.h"

enum symbol_id {
    SYMBOL_X=0,
    SYMBOL_MINIMIZE=1,
    SYMBOL_DROPDOWN=2
};

void init_symbol_table(Window *win);


#endif
