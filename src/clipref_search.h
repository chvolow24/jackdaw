#ifndef JDAW_CLIPREF_SEARCH_H
#define JDAW_CLIPREF_SEARCH_H

#include "project.h"

void clipref_insert_sorted(ClipRef *insertion, ClipRef **cr_list, uint16_t *num_clips, bool by_end);
void clipref_list_fwdsort(ClipRef **list, uint16_t num_clips);
void clipref_list_backsort(ClipRef **list, uint16_t num_clips);
#endif
