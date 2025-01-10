#ifndef JDAW_COMPONENTS_UNDO_H
#define JDAW_COMPONENTS_UNDO_H

#include "components.h"

void radio_push_event(RadioButton *rb, int newval);
void push_drag_event(Draggable *d, Value current_val);
void toggle_push_event(Toggle *tgl);

#endif
