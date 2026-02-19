#ifndef SYNTH_PAGE_H
#define SYNTH_PAGE_H

#include "page.h"
#include "project.h"

TabView *synth_tabview_create(Track *track);

void synth_open_preset();
void synth_save_preset();

#endif
