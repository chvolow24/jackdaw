/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "endpoint_callbacks.h"
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

void track_slider_cb(Endpoint *ep)
{
    Slider *s = *((Slider **)ep->xarg1);
    Value val = slider_reset(s);
    label_reset(s->label, val);
    Timeline *tl = (Timeline *)ep->xarg2;
    tl->needs_redraw = true;
}



/* static PageEl *track_settings_get_el(const char *id) */
/* { */
/*     TabView *tv = main_win->active_tabview; */
/*     if (!tv) return NULL; */
/*     PageEl *ret = NULL; */
/*     for (int i=0; i<tv->num_tabs; i++) { */
/* 	Page *tab = tv->tabs[i]; */
/* 	if ((ret = page_get_el_by_id(tab, id))) { */
/* 	    break; */
/* 	} */
/*     } */
/*     return ret; */
/* } */


/* void filter_cutoff_gui_cb(Endpoint *ep) */
/* { */
/*     /\* PageEl *el = track_settings_get_el("track_settings_filter_cutoff_slider"); *\/ */
/*     if (!el) return; */
/*     Slider *s = (Slider *)el->component; */
/*     Value val = slider_reset(s); */
/*     label_reset(s->label, val); */
/* } */


/* void filter_bandwidth_gui_cb(Endpoint *ep) */
/* { */
/*     PageEl *el = track_settings_get_el("track_settings_filter_bandwidth_slider"); */
/*     if (!el) return; */
/*     Slider *s = (Slider *)el->component; */
/*     Value val = slider_reset(s); */
/*     label_reset(s->label, val); */
/* } */

/* /\* void settings_reset_freq_plot(struct freq_plot *fp,  *\/ */

/* void filter_irlen_gui_cb(Endpoint *ep) */
/* { */
    
/*     PageEl *el = track_settings_get_el("track_settings_filter_irlen_slider"); */
/*     if (el) { */
/* 	Slider *s = (Slider *)el->component; */
/* 	Value val = slider_reset(s); */
/* 	label_reset(s->label, val); */
/*     } */
/* } */

/* void filter_type_gui_cb(Endpoint *ep) */
/* { */
/*     PageEl *el = track_settings_get_el("track_settings_filter_type_radio"); */
/*     if (!el) return; */
/*     radio_button_reset_from_endpoint((RadioButton *)el->component);    */
    
/* } */

/* void saturation_gain_gui_cb(Endpoint *ep) */
/* { */
/*     PageEl *el = track_settings_get_el("track_settings_saturation_gain"); */
/*     if (!el) return; */
/*     Slider *s = (Slider *)el->component; */
/*     Value val = slider_reset(s); */
/*     label_reset(s->label, val); */

    
/* } */
/* void saturation_type_gui_cb(Endpoint *ep) */
/* { */
/*     PageEl *el = track_settings_get_el("track_settings_saturation_type"); */
/*     if (!el) return; */
/*     radio_button_reset_from_endpoint((RadioButton *)el->component);    */
/* } */

void page_el_gui_cb(Endpoint *ep)
{
    /* if (!main_win->active_tabview) return; */

    Page **page_loc = ep->xarg3;
    if (!page_loc) {
	fprintf(stderr, "Error: track settings callback: endpoint does not contain page loc in third xarg\n");
	return;
    }
    Page *page = *page_loc;
    if (!page) {
	/* fprintf(stderr, "(page not active)\n"); */
	return;
    }
    TabView *tv = main_win->active_tabview;
    if (!tv || tv->tabs[tv->current_tab] != page) {
	return;
    }
    const char *el_id = ep->xarg4;

    PageEl *el = page_get_el_by_id(page, el_id);
    if (!el) {
	fprintf(stderr, "Error: gui callback for endpoint \"%s\" failed; page element with id \"%s\" not found in page \"%s\".\n", ep->display_name, el_id, page->title);
	return;
    }
    page_el_reset(el);
}


/* static void secondary_delay_line_gui_cb(Endpoint *ep) */
/* { */
/*     /\* fprintf(stderr, "AND GUI at %lu\n", clock()); *\/ */
/*     PageEl *el = track_settings_get_el("track_settings_delay_time_slider"); */
/*     if (el) { */
/* 	slider_reset(el->component); */
/*     } */

/* } */

/* void delay_line_len_gui_cb(Endpoint *ep) */
/* { */
/*     PageEl *el = track_settings_get_el("track_settings_delay_time_slider"); */
/*     if (el) { */
/* 	Slider *s = (Slider *)el->component; */
/* 	Value val = slider_reset(s); */
/* 	label_reset(s->label, val); */

/*     } */

/* } */

/* void delay_line_amp_gui_cb(Endpoint *ep) */
/* { */
/*     PageEl *el = track_settings_get_el("track_settings_delay_amp_slider"); */
/*     if (el) { */
/* 	Slider *s = (Slider *)el->component; */
/* 	Value val = slider_reset(s); */
/* 	label_reset(s->label, val); */

/*     } */
/* } */

/* void delay_line_stereo_offset_gui_cb(Endpoint *ep) */
/* { */
/*     PageEl *el = track_settings_get_el("track_settings_delay_stereo_offset_slider"); */
/*     if (el) { */
/* 	Slider *s = (Slider *)el->component; */
/* 	Value val = slider_reset(s); */
/* 	label_reset(s->label, val); */
/*     } */
    
/* } */

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
    Value new_pos = endpoint_safe_read(ep, NULL);
    click_segment_set_start_pos(s, new_pos.int32_v);
}

void click_segment_bound_gui_cb(Endpoint *ep)
{

}


