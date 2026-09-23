/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "endpoint_callbacks.h"
#include "components.h"
#include "dev.h"
#include "modal.h"
#include "page.h"
#include "session_endpoint_ops.h"
#include "status.h"

extern Project *proj;
extern Window *main_win;

void play_speed_gui_cb(Endpoint *ep)
{
    status_stat_playspeed();
}


/* Use component_gui_cb instead; component must be initialized w/ endpoint */
DEPRECATED void page_el_gui_cb(Endpoint *ep)
{
    Page **page_loc = ep->xarg3;
    /* char dststr[32]; */
    /* jdaw_val_to_str(dststr, 32, endpoint_read(ep, NULL), ep->val_type, 2); */
    /* fprintf(stderr, "PAGE EL cb %s\n", dststr); */
    if (!page_loc) {
	fprintf(stderr, "Error: track settings callback: endpoint does not contain page loc in third xarg\n");
	return;
    }
    Page *page = *page_loc;
    if (!page) {
	/* fprintf(stderr, "(page not active)\n"); */
	return;
    }

    bool page_in_pa = false;
    Session *session = session_get();
    for (int i=0; i<session->gui.panels->num_pages; i++) {
	if (page == session->gui.panels->pages[i]) {
	    page_in_pa = true;
	    break;
	}
    }
    if (!page_in_pa) {
	TabView *tv = main_win->active_tabview;
	if (!tv || tv->tabs[tv->current_tab] != page) {
	    return;
	}
    }
    const char *el_id = ep->xarg4;

    PageEl *el = page_get_el_by_id(page, el_id);
    if (!el) {
	fprintf(stderr, "Error: gui callback for endpoint \"%s\" failed; page element with id \"%s\" not found in page \"%s\".\n", ep->display_name, el_id, page->title);
	return;
    }
    page_el_reset(el);
}

void component_gui_cb(Endpoint *ep)
{
    if (!ep->bound_component) {
	return;
    }
    switch (ep->bound_component_type) {
    case EL_SLIDER:
	slider_reset(ep->bound_component);
	break;
    case EL_RADIO:
	radio_button_reset_from_endpoint(ep->bound_component);
	break;
    case EL_DROPDOWN:
	dropdown_reset(ep->bound_component);
	break;
    case EL_TOGGLE:
	break;
    default:
	break;
    }
}

void click_track_ebb_gui_cb(Endpoint *ep)
{
    if (main_win->num_modals > 0) {
	Modal *m = main_win->modals[main_win->num_modals - 1];
	for (int i=0; i<m->num_els; i++) {
	    ModalEl *el = m->els[i];
	    if (el->type == MODAL_EL_RADIO) {
		RadioButton *rb = el->obj;
		if (rb->ep == ep) {
		    radio_button_reset_from_endpoint(rb);
		}
	    }
	}
    } else if (main_win->active_tabview) {
	Page *p = main_win->active_tabview->tabs[0];
	PageEl *el = page_get_el_by_id(p, "click_segment_ebb_radio");
	if (el) {
	    radio_button_reset_from_endpoint(el->component);
	}
    }
}


void click_segment_set_start_pos(ClickSegment *s, int32_t new_end_pos);
void click_segment_bound_proj_cb(Endpoint *ep)
{
    ClickSegment *s = ep->xarg1;
    Value new_pos = endpoint_read(ep, NULL);
    click_segment_set_start_pos(s, new_pos.int32_v);
}

void click_segment_bound_gui_cb(Endpoint *ep)
{

}


