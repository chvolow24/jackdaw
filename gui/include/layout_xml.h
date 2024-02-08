#ifndef JDAW_GUI_LT_XML_H
#define JDAW_GUI_LT_XML_H

#include "layout.h"

Layout *read_layout_from_xml(const char *filename);

void write_layout(FILE *f, Layout *lt, int indent);
void write_layout_to_file(Layout *lt);
void write_layout_to_file(Layout *lt);
Layout *read_xml_to_lt(Layout *dst, char *filepath);

#endif