#ifndef JDAW_AUDIO_ROUTE_PAGE_H
#define JDAW_AUDIO_ROUTE_PAGE_H

#include <stdbool.h>
typedef struct track Track;

void route_quick_add(Track *trck, bool trck_is_dst);
void route_page_open(Track *track, bool select_outs_tab);

#endif
