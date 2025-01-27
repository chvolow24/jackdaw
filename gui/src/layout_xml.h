#ifndef JDAW_GUI_LT_XML_H
#define JDAW_GUI_LT_XML_H

#include "layout.h"

Layout *layout_read_from_xml(const char *filename);

void layout_write(FILE *f, Layout *lt, int indent);
void layout_write_debug(FILE *f, Layout *lt, int indent, int maxdepth);
void layout_write_to_file(Layout *lt);

Layout *layout_read_xml_to_lt(Layout *dst, char *filepath);

#endif
