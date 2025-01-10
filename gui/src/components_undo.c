#include "components.h"
#include "project.h"
#include "user_event.h"

extern Project *proj;

/* bool toggle_toggle_internal(Toggle *toggle, bool from_undo); */

NEW_EVENT_FN(undo_redo_toggle, "undo/redo toggle")
    Toggle *tgl = (Toggle *)obj1;
    toggle_toggle_internal(tgl, true);
}

NEW_EVENT_FN(dispose_toggle, "")
    Toggle *tgl = (Toggle *)obj1;
    toggle_decr_undo_refs(tgl);
}
void toggle_push_event(Toggle *tgl)
{
    tgl->undo_state.num_refs++;
    Value nullval = {.int_v = 0};
    user_event_push(
	&proj->history,
	undo_redo_toggle,
	undo_redo_toggle,
	dispose_toggle, dispose_toggle,
	(void *)tgl, NULL,
	nullval, nullval,
	nullval, nullval,
	0, 0, false, false);
}

NEW_EVENT_FN(undo_redo_radio_select, "undo/redo radio select")
    RadioButton *rb = (RadioButton *)obj1;
    radio_select_internal(rb, val1.int_v, true);
}
NEW_EVENT_FN(dispose_radio_select, "")
    RadioButton *rb = (RadioButton *)obj1;
    radio_decr_undo_refs(rb);
}

void radio_push_event(RadioButton *rb, int newval)
{
    rb->undo_state.num_refs++;
    Value undo_val = {.int_v = (int)rb->selected_item};
    Value redo_val = {.int_v = newval};
    user_event_push(
	&proj->history,
	undo_redo_radio_select,
        undo_redo_radio_select,
	dispose_radio_select, dispose_radio_select,
	(void *)rb, NULL,
	undo_val, undo_val,
	redo_val, redo_val,
	0, 0, false, false);
}



NEW_EVENT_FN(undo_redo_drag_event, "undo drag event")
    switch ((enum drag_comp_type)val1.int_v) {
    case DRAG_SLIDER:
	slider_set_value((Slider *)obj1, val2);
	break;
    case DRAG_TEMPO_SEG_BOUND:
	break;
    default:
	break;
    }
}

NEW_EVENT_FN(dispose_drag_event, "")
    if ((enum drag_comp_type)val1.int_v == DRAG_SLIDER) {
	Slider *s = (Slider *)obj1;
	slider_decr_undo_refs(s);
    }
}
void push_drag_event(Draggable *d, Value current_val)
{
    Value comptype = {.int_v = (int)d->type};
    if (d->type == DRAG_SLIDER) {
	Slider *s = (Slider *)d->component;
	s->undo_state.num_refs++;
    }
    user_event_push(
	&proj->history,
	undo_redo_drag_event,
	undo_redo_drag_event,
	dispose_drag_event, dispose_drag_event,
	d->component, NULL,
	comptype, d->cached_val,
	comptype, current_val,
	d->cached_val_type, d->cached_val_type,
	false, false);	
}

