#include "color.h"
#include "layout.h"
#include "page.h"
#include "session.h"
#include "synth.h"

extern Window *main_win;

extern struct colors colors;

static void add_osc_page(TabView *tv, Track *track);
static void add_filter_page(TabView *tv, Track *track);

void osc_bckgrnd_draw(void *arg1, void *arg2);

TabView *synth_tabview_create(Track *track)
{
    Session *session = session_get();
    TabView *tv = tabview_create(
	"Synth settings",
	session->gui.layout,
	main_win);
    add_osc_page(tv, track);
    add_filter_page(tv, track);
    return tv;
}

static void page_fill_out_layout(Page *p);

static void add_osc_page(TabView *tv, Track *track)
{
    
    Synth *s = track->synth;
    static SDL_Color osc_bckgrnd = {40, 40, 40, 255};
    Page *page = tabview_add_page(tv, "Oscillators", SYNTH_OSCS_LT_PATH, &osc_bckgrnd, &colors.white, main_win);


    /* Fill out layout */
    page_fill_out_layout(page);


    
    union page_el_params p;
    PageEl *el = NULL;

    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 14;
    p.textbox_p.set_str = "Vol:";
    p.textbox_p.win = main_win;
    el = page_add_el(page,EL_TEXTBOX,p,"","master_amp_label");
    
    p.textbox_p.set_str = "Pan:";
    el = page_add_el(page,EL_TEXTBOX,p,"","master_pan_label");

    p.textbox_p.font = main_win->mono_font;
    p.textbox_p.set_str = "Osc 1";
    el = page_add_el(page,EL_TEXTBOX,p,"","osc1_label");

    p.textbox_p.set_str = "Osc 2";
    el = page_add_el(page,EL_TEXTBOX,p,"","osc2_label");

    p.textbox_p.set_str = "Osc 3";
    el = page_add_el(page,EL_TEXTBOX,p,"","osc3_label");

    p.textbox_p.set_str = "Osc 4";
    el = page_add_el(page,EL_TEXTBOX,p,"","osc4_label");

    p.textbox_p.set_str = "Vol:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1vol_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","2vol_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","3vol_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","4vol_label");

    Textbox *tb;
    p.textbox_p.set_str = "Pan:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1pan_label");
    /* tb = el->component; */
    /* textbox_set_trunc(tb, false); */
    el = page_add_el(page,EL_TEXTBOX,p,"","2pan_label");
    /* tb = el->component; */
    /* textbox_set_trunc(tb, false); */
    el = page_add_el(page,EL_TEXTBOX,p,"","3pan_label");
    /* tb = el->component; */
    /* textbox_set_trunc(tb, false); */
    el = page_add_el(page,EL_TEXTBOX,p,"","4pan_label");
    /* tb = el->component; */
    /* textbox_set_trunc(tb, false); */

    p.textbox_p.set_str = "Oct:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1octave_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","2octave_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","3octave_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","4octave_label");

    p.textbox_p.set_str = "Coarse:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1coarse_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","2coarse_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","3coarse_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","4coarse_label");

    p.textbox_p.set_str = "Fine:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1fine_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","2fine_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","3fine_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","4fine_label");


    p.textbox_p.set_str = "Voices:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1unison_num_voices_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","2unison_num_voices_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","3unison_num_voices_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","4unison_num_voices_label");


    p.textbox_p.set_str = "Detune:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1unison_detune_cents_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","2unison_detune_cents_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","3unison_detune_cents_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","4unison_detune_cents_label");

    p.textbox_p.set_str = "Rel Vol:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1unison_relative_amp_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","2unison_relative_amp_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","3unison_relative_amp_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","4unison_relative_amp_label");
    
    p.textbox_p.set_str = "Stereo:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1unison_stereo_spread_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","2unison_stereo_spread_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","3unison_stereo_spread_label");
    el = page_add_el(page,EL_TEXTBOX,p,"","4unison_stereo_spread_label");



	
}

static void add_filter_page(TabView *tv, Track *track)
{

}



/* Garbage Function */
static void page_fill_out_layout(Page *page)
{
    for (int o=0; o<4; o++) {
	Layout *tune_panel = layout_get_child_by_name_recursive(page->layout, "tune_panel");
	Layout *unison_panel = layout_get_child_by_name_recursive(page->layout, "unison_panel");

	char name[255];
	snprintf(name, 255, "%dtune_panel", o+1);
	layout_set_name(tune_panel, name);
	snprintf(name, 255, "%dunison_panel", o+1);
	layout_set_name(unison_panel, name);
	for (int i=0; i<3; i++) {
	    Layout *row_lt = layout_add_child(tune_panel);
	    row_lt->w.type = SCALE;
	    row_lt->w.value = 1.0;
	    row_lt->h.type = SCALE;
	    row_lt->h.value = 0.3333333;
	    row_lt->y.type = STACK;

	    Layout *label_lt = layout_add_child(row_lt);
	    label_lt->w.type = SCALE;
	    label_lt->w.value = 0.3;
	    label_lt->h.type = SCALE;
	    label_lt->h.value = 1.0;

	    snprintf(name, 255, "%d%s_label", o+1, i==0 ? "octave" : i==1 ? "coarse" : "fine");
	    layout_set_name(label_lt, name);
	    Layout *slider_lt = layout_add_child(row_lt);
	    slider_lt->w.type = SCALE;
	    slider_lt->w.value = 0.7;
	    slider_lt->h.type = SCALE;
	    slider_lt->h.value = 0.7;
	    slider_lt->x.type = STACK;
	    slider_lt->y.type = SCALE;
	    slider_lt->y.value = 0.15;
	    snprintf(name, 255, "%d%s_slider", o+1, i==0 ? "octave" : i==1 ? "coarse" : "fine");
	    layout_set_name(slider_lt, name);

	}
	for (int i=0; i<4; i++) {
	    Layout *row_lt = layout_add_child(unison_panel);
	    row_lt->w.type = SCALE;
	    row_lt->w.value = 1.0;
	    row_lt->h.type = SCALE;
	    row_lt->h.value = 0.25;
	    row_lt->y.type = STACK;

	    Layout *label_lt = layout_add_child(row_lt);
	    label_lt->w.type = SCALE;
	    label_lt->w.value = 0.3;
	    label_lt->h.type = SCALE;
	    label_lt->h.value = 1.0;
	    snprintf(name, 255, "%d%s_label", o+1, i==0 ? "unison_num_voices" : i==1 ? "unison_detune_cents" : i==2 ?  "unison_relative_amp" : "unison_stereo_spread");
	    layout_set_name(label_lt, name);


	    Layout *slider_lt = layout_add_child(row_lt);
	    slider_lt->w.type = SCALE;
	    slider_lt->w.value = 0.7;
	    slider_lt->h.type = SCALE;
	    slider_lt->h.value = 0.7;
	    slider_lt->x.type = STACK;
	    slider_lt->y.type = SCALE;
	    slider_lt->y.value = 0.15;
	    snprintf(name, 255, "%d%s_slider", o+1, i==0 ? "unison_num_voices" : i==1 ? "unison_detune_cents" : i==2 ?  "unison_relative_amp" : "unison_stereo_spread");
	    layout_set_name(slider_lt, name);
	}
    }

    layout_force_reset(page->layout);
    /* FILE *f = fopen("_synth_oscs.xml", "w"); */
    /* layout_write(f, page->layout, 0); */
    /* fclose(f); */
    /* exit(0); */


}
