#ifndef JDAW_GUI_OPENFILE_H
#define JDAW_GUI_OPENFILE_H

#include "text.h"

typedef struct openfile {
    Text *label;
    Text *filepath;
} OpenFile;

Layout *openfile_loop(Layout *lt);
#endif