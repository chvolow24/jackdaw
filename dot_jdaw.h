#ifndef JDAW_DOT_JDAW_H
#define JDAW_DOT_JDAW_H

#include "project.h"

void write_jdaw_file(const char *path);
Project *open_jdaw_file(const char *path);

#endif