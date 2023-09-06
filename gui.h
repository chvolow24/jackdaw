#ifndef JDAW_GUI_H
#define JDAW_GUI_H

#include "project.h"

typedef struct project {
    char name[MAX_NAMELENGTH];
    Timeline tl;
    bool dark_mode;
} Project;

#endif