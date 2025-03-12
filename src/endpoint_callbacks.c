/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    endpoint_callbacks.c

    * Definitions of all endpoint callback functions
 *****************************************************************************************************************/

#include "dsp.h"
#include "endpoint_callbacks.h"
#include "page.h"
#include "project_endpoint_ops.h"
#include "status.h"
#include "waveform.h"

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

void filter_cutoff_dsp_cb(Endpoint *ep)
{
    FIRFilter *f = (FIRFilter *)ep->xarg1;
    Value cutoff = endpoint_safe_read(ep, NULL);
    double cutoff_hz = dsp_scale_freq_to_hz(cutoff.double_v);
    filter_set_cutoff_hz(f, cutoff_hz);
    /* fprintf(stderr, "DSP callback\n"); */
}

static PageEl *track_settings_get_el(const char *id)
{
    TabView *tv = main_win->active_tabview;
    if (!tv) return NULL;
    PageEl *ret = NULL;
    for (int i=0; i<tv->num_tabs; i++) {
	Page *tab = tv->tabs[i];
	if ((ret = page_get_el_by_id(tab, id))) {
	    break;
	}
    }
    return ret;
}


void filter_cutoff_gui_cb(Endpoint *ep)
{
    PageEl *el = track_settings_get_el("track_settings_filter_cutoff_slider");
    if (!el) return;
    Slider *s = (Slider *)el->component;
    Value val = slider_reset(s);
    label_reset(s->label, val);

}

void filter_bandwidth_dsp_cb(Endpoint *ep)
{
    FIRFilter *f = (FIRFilter *)ep->xarg1;
    Value bandwidth = endpoint_safe_read(ep, NULL);
    double bandwidth_hz = dsp_scale_freq_to_hz(bandwidth.double_v);
    filter_set_bandwidth_hz(f, bandwidth_hz);
}

void filter_bandwidth_gui_cb(Endpoint *ep)
{
    PageEl *el = track_settings_get_el("track_settings_filter_bandwidth_slider");
    if (!el) return;
    Slider *s = (Slider *)el->component;
    Value val = slider_reset(s);
    label_reset(s->label, val);
}

/* void settings_reset_freq_plot(struct freq_plot *fp,  */
void filter_irlen_dsp_cb(Endpoint *ep)
{
    FIRFilter *f = (FIRFilter *)ep->xarg1;
    Value irlen_val = endpoint_safe_read(ep, NULL);
    filter_set_impulse_response_len(f, irlen_val.uint16_v);

    /* fprintf(stderr, "RESETTING mag\n"); */
    /* PageEl *el = track_settings_get_el("track_settings_filter_freq_plot"); */
    /* if (!el) return; */
    /* struct freq_plot *fp = (struct freq_plot *)el->component; */
    /* pthread_mutex_lock(&f->lock); */
    /* fp->arrays[2] = f->frequency_response_mag; */
    /* waveform_reset_freq_plot(fp); */
    /* pthread_mutex_unlock(&f->lock); */
    /* fprintf(stderr, "RESET WAVEFORM\n"); */
}

void filter_irlen_gui_cb(Endpoint *ep)
{
    
    PageEl *el = track_settings_get_el("track_settings_filter_irlen_slider");
    if (el) {
	Slider *s = (Slider *)el->component;
	Value val = slider_reset(s);
	label_reset(s->label, val);
    }
}

void filter_type_gui_cb(Endpoint *ep)
{
    PageEl *el = track_settings_get_el("track_settings_filter_type_radio");
    if (!el) return;
    radio_button_reset_from_endpoint((RadioButton *)el->component);   
    
}
void filter_type_dsp_cb(Endpoint *ep)
{
    FIRFilter *f = (FIRFilter *)ep->xarg1;
    Value val = endpoint_safe_read(ep, NULL);
    FilterType t = (FilterType)val.int_v;
    filter_set_type(f, t);
    
}

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

void track_settings_page_el_gui_cb(Endpoint *ep)
{
    const char *el_id = ep->xarg4;
    PageEl *el = track_settings_get_el(el_id);
    if (!el) return;
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
void delay_line_len_dsp_cb(Endpoint *ep)
{
    DelayLine *dl = (DelayLine *)ep->xarg1;
    int16_t val_msec = endpoint_safe_read(ep, NULL).int16_v;
    int32_t len_sframes = (int32_t)((double)val_msec * proj->sample_rate / 1000.0);
    delay_line_set_params(dl, dl->amp, len_sframes);
    /* project_queue_callback(proj, ep, secondary_delay_line_gui_cb, JDAW_THREAD_MAIN); */
}

void delay_line_len_gui_cb(Endpoint *ep)
{
    PageEl *el = track_settings_get_el("track_settings_delay_time_slider");
    if (el) {
	Slider *s = (Slider *)el->component;
	Value val = slider_reset(s);
	label_reset(s->label, val);

    }

}

void delay_line_amp_gui_cb(Endpoint *ep)
{
    PageEl *el = track_settings_get_el("track_settings_delay_amp_slider");
    if (el) {
	Slider *s = (Slider *)el->component;
	Value val = slider_reset(s);
	label_reset(s->label, val);

    }
}

void delay_line_stereo_offset_gui_cb(Endpoint *ep)
{
    PageEl *el = track_settings_get_el("track_settings_delay_stereo_offset_slider");
    if (el) {
	Slider *s = (Slider *)el->component;
	Value val = slider_reset(s);
	label_reset(s->label, val);
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
    Value new_pos = endpoint_safe_read(ep, NULL);
    click_segment_set_start_pos(s, new_pos.int32_v);
}

void click_segment_bound_gui_cb(Endpoint *ep)
{

}


