#include "color.h"
#include "layout.h"
#include "page.h"
#include "session.h"
#include "synth.h"

extern Window *main_win;

extern struct colors colors;
extern Symbol *SYMBOL_TABLE[];

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

    s->osc_page = page;
    /* Fill out layout */
    page_fill_out_layout(page);


    
    union page_el_params p;
    PageEl *el = NULL;

    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 14;
    p.textbox_p.win = main_win;

    p.textbox_p.set_str = "Vol:";
    page_add_el(page,EL_TEXTBOX,p,"","master_amp_label");
    
    p.textbox_p.set_str = "Pan:";
    page_add_el(page,EL_TEXTBOX,p,"","master_pan_label");

    p.textbox_p.font = main_win->mono_font;
    p.textbox_p.set_str = "Osc 1";
    page_add_el(page,EL_TEXTBOX,p,"","osc1_label");

    p.textbox_p.set_str = "Osc 2";
    page_add_el(page,EL_TEXTBOX,p,"","osc2_label");

    p.textbox_p.set_str = "Osc 3";
    page_add_el(page,EL_TEXTBOX,p,"","osc3_label");

    p.textbox_p.set_str = "Osc 4";
    page_add_el(page,EL_TEXTBOX,p,"","osc4_label");

    p.textbox_p.set_str = "Vol:";
    page_add_el(page,EL_TEXTBOX,p,"","1vol_label");
    page_add_el(page,EL_TEXTBOX,p,"","2vol_label");
    page_add_el(page,EL_TEXTBOX,p,"","3vol_label");
    page_add_el(page,EL_TEXTBOX,p,"","4vol_label");

    Textbox *tb;
    p.textbox_p.set_str = "Pan:";
    page_add_el(page,EL_TEXTBOX,p,"","1pan_label");
    /* tb = el->component; */
    /* textbox_set_trunc(tb, false); */
    page_add_el(page,EL_TEXTBOX,p,"","2pan_label");
    /* tb = el->component; */
    /* textbox_set_trunc(tb, false); */
    page_add_el(page,EL_TEXTBOX,p,"","3pan_label");
    /* tb = el->component; */
    /* textbox_set_trunc(tb, false); */
    page_add_el(page,EL_TEXTBOX,p,"","4pan_label");
    /* tb = el->component; */
    /* textbox_set_trunc(tb, false); */

    p.textbox_p.set_str = "Octave:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1octave_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","2octave_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","3octave_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","4octave_label");
    layout_size_to_fit_children_h(el->layout, true, 0);

    p.textbox_p.set_str = "Coarse:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1coarse_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","2coarse_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","3coarse_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","4coarse_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    layout_reset(page->layout);
    /* layout_write(stderr, el->layout, 0); */
    /* fprintf(stderr, "\n\n Parent!\n"); */
    /* FILE *fl = fopen("l.xml", "w"); */
    /* layout_write(fl, el->layout->parent,0); */
    /* fclose(fl); */
    /* exit(1); */

    p.textbox_p.set_str = "Fine:";
    page_add_el(page,EL_TEXTBOX,p,"","1fine_label");
    page_add_el(page,EL_TEXTBOX,p,"","2fine_label");
    page_add_el(page,EL_TEXTBOX,p,"","3fine_label");
    page_add_el(page,EL_TEXTBOX,p,"","4fine_label");


    p.textbox_p.set_str = "Voices:";
    page_add_el(page,EL_TEXTBOX,p,"","1unison_num_voices_label");
    page_add_el(page,EL_TEXTBOX,p,"","2unison_num_voices_label");
    page_add_el(page,EL_TEXTBOX,p,"","3unison_num_voices_label");
    page_add_el(page,EL_TEXTBOX,p,"","4unison_num_voices_label");


    p.textbox_p.set_str = "Detune:";
    page_add_el(page,EL_TEXTBOX,p,"","1unison_detune_cents_label");
    page_add_el(page,EL_TEXTBOX,p,"","2unison_detune_cents_label");
    page_add_el(page,EL_TEXTBOX,p,"","3unison_detune_cents_label");
    page_add_el(page,EL_TEXTBOX,p,"","4unison_detune_cents_label");

    p.textbox_p.set_str = "Rel Vol:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1unison_relative_amp_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","2unison_relative_amp_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","3unison_relative_amp_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","4unison_relative_amp_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    layout_reset(page->layout);
    
    p.textbox_p.set_str = "Stereo:";
    page_add_el(page,EL_TEXTBOX,p,"","1unison_stereo_spread_label");
    page_add_el(page,EL_TEXTBOX,p,"","2unison_stereo_spread_label");
    page_add_el(page,EL_TEXTBOX,p,"","3unison_stereo_spread_label");
    page_add_el(page,EL_TEXTBOX,p,"","4unison_stereo_spread_label");


    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_FILL;
    /* page_el_params_slider_from_ep(&p, &s->vol_ep); */

    /* page_add_el(page,EL_SLIDER,p,"synth_vol_slider","master_amp_slider"); */

    /* page_el_params_slider_from_ep(&p, &s->pan_ep); */
    /* page_add_el(page,EL_SLIDER,p,"synth_pan_slider","master_pan_slider"); */
    /* for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) { */
    OscCfg *cfg = s->base_oscs;
	
    page_el_params_slider_from_ep(&p, &cfg->amp_ep);	
    page_add_el(page,EL_SLIDER,p,"1vol","1vol_slider");

    p.slider_p.style = SLIDER_TICK;
    page_el_params_slider_from_ep(&p, &cfg->pan_ep);
    page_add_el(page,EL_SLIDER,p,"1pan","1pan_slider");
    
    page_el_params_slider_from_ep(&p, &cfg->octave_ep);
    page_add_el(page,EL_SLIDER,p,"1octave","1octave_slider");

    page_el_params_slider_from_ep(&p, &cfg->tune_coarse_ep);
    page_add_el(page,EL_SLIDER,p,"1tune_coarse","1coarse_slider");

    page_el_params_slider_from_ep(&p, &cfg->tune_fine_ep);
    page_add_el(page,EL_SLIDER,p,"1tune_fine","1fine_slider");

    p.slider_p.style = SLIDER_FILL;
    page_el_params_slider_from_ep(&p, &cfg->unison.num_voices_ep);
    page_add_el(page,EL_SLIDER,p,"1num_voices","1unison_num_voices_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.detune_cents_ep);
    page_add_el(page,EL_SLIDER,p,"1detune_cents","1unison_detune_cents_slider");
    
    page_el_params_slider_from_ep(&p, &cfg->unison.relative_amp_ep);
    page_add_el(page,EL_SLIDER,p,"1relative_amp","1unison_relative_amp_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.stereo_spread_ep);
    page_add_el(page,EL_SLIDER,p,"1stereo_spread","1unison_stereo_spread_slider");



    cfg++;
    page_el_params_slider_from_ep(&p, &cfg->amp_ep);	
    page_add_el(page,EL_SLIDER,p,"2vol","2vol_slider");

    p.slider_p.style = SLIDER_TICK;
    page_el_params_slider_from_ep(&p, &cfg->pan_ep);
    page_add_el(page,EL_SLIDER,p,"2pan","2pan_slider");
    
    page_el_params_slider_from_ep(&p, &cfg->octave_ep);
    page_add_el(page,EL_SLIDER,p,"2octave","2octave_slider");

    page_el_params_slider_from_ep(&p, &cfg->tune_coarse_ep);
    page_add_el(page,EL_SLIDER,p,"2tune_coarse","2coarse_slider");

    page_el_params_slider_from_ep(&p, &cfg->tune_fine_ep);
    page_add_el(page,EL_SLIDER,p,"2tune_fine","2fine_slider");

    p.slider_p.style = SLIDER_FILL;
    page_el_params_slider_from_ep(&p, &cfg->unison.num_voices_ep);
    page_add_el(page,EL_SLIDER,p,"2num_voices","2unison_num_voices_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.detune_cents_ep);
    page_add_el(page,EL_SLIDER,p,"2detune_cents","2unison_detune_cents_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.relative_amp_ep);
    page_add_el(page,EL_SLIDER,p,"2relative_amp","2unison_relative_amp_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.stereo_spread_ep);
    page_add_el(page,EL_SLIDER,p,"2stereo_spread","2unison_stereo_spread_slider");


    cfg++;
    page_el_params_slider_from_ep(&p, &cfg->amp_ep);	
    page_add_el(page,EL_SLIDER,p,"3vol","3vol_slider");

    p.slider_p.style = SLIDER_TICK;
    page_el_params_slider_from_ep(&p, &cfg->pan_ep);
    page_add_el(page,EL_SLIDER,p,"3pan","3pan_slider");
    
    page_el_params_slider_from_ep(&p, &cfg->octave_ep);
    page_add_el(page,EL_SLIDER,p,"3octave","3octave_slider");

    page_el_params_slider_from_ep(&p, &cfg->tune_coarse_ep);
    page_add_el(page,EL_SLIDER,p,"3tune_coarse","3coarse_slider");

    page_el_params_slider_from_ep(&p, &cfg->tune_fine_ep);
    page_add_el(page,EL_SLIDER,p,"3tune_fine","3fine_slider");

    p.slider_p.style = SLIDER_FILL;
    page_el_params_slider_from_ep(&p, &cfg->unison.num_voices_ep);
    page_add_el(page,EL_SLIDER,p,"3num_voices","3unison_num_voices_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.detune_cents_ep);
    page_add_el(page,EL_SLIDER,p,"3detune_cents","3unison_detune_cents_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.relative_amp_ep);
    page_add_el(page,EL_SLIDER,p,"3relative_amp","3unison_relative_amp_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.stereo_spread_ep);
    page_add_el(page,EL_SLIDER,p,"3stereo_spread","3unison_stereo_spread_slider");


    cfg++;
    page_el_params_slider_from_ep(&p, &cfg->amp_ep);	
    page_add_el(page,EL_SLIDER,p,"4vol","4vol_slider");

    p.slider_p.style = SLIDER_TICK;
    page_el_params_slider_from_ep(&p, &cfg->pan_ep);
    page_add_el(page,EL_SLIDER,p,"4pan","4pan_slider");
    
    page_el_params_slider_from_ep(&p, &cfg->octave_ep);
    page_add_el(page,EL_SLIDER,p,"4octave","4octave_slider");

    page_el_params_slider_from_ep(&p, &cfg->tune_coarse_ep);
    page_add_el(page,EL_SLIDER,p,"4tune_coarse","4coarse_slider");

    page_el_params_slider_from_ep(&p, &cfg->tune_fine_ep);
    page_add_el(page,EL_SLIDER,p,"4tune_fine","4fine_slider");

    
    p.slider_p.style = SLIDER_FILL;
    page_el_params_slider_from_ep(&p, &cfg->unison.num_voices_ep);
    page_add_el(page,EL_SLIDER,p,"4num_voices","4unison_num_voices_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.detune_cents_ep);
    page_add_el(page,EL_SLIDER,p,"4detune_cents","4unison_detune_cents_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.relative_amp_ep);
    page_add_el(page,EL_SLIDER,p,"4relative_amp","4unison_relative_amp_slider");

    page_el_params_slider_from_ep(&p, &cfg->unison.stereo_spread_ep);
    page_add_el(page,EL_SLIDER,p,"4stereo_spread","4unison_stereo_spread_slider");



    static SDL_Color sel_color = {0, 255, 0, 150};
    p.sradio_p.align_horizontal = true;
    p.sradio_p.symbols = SYMBOL_TABLE + SYMBOL_SINE;
    p.sradio_p.num_items = 4;
    p.sradio_p.padding = 7;
    p.sradio_p.unsel_color = &colors.clear;
    p.sradio_p.sel_color = &sel_color;
    p.sradio_p.ep = &cfg->type_ep;
    page_add_el(page,EL_SYMBOL_RADIO,p,"4type_radio","4type_radio");
    /* } */



    layout_force_reset(page->layout);


	
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
	    slider_lt->w.type = COMPLEMENT;
	    /* slider_lt->w.type = SCALE; */
	    /* slider_lt->w.value = 0.7; */
	    slider_lt->h.type = SCALE;
	    slider_lt->h.value = 0.525;
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
	    label_lt->w.value = 0.4;
	    label_lt->h.type = SCALE;
	    label_lt->h.value = 1.0;
	    snprintf(name, 255, "%d%s_label", o+1, i==0 ? "unison_num_voices" : i==1 ? "unison_detune_cents" : i==2 ?  "unison_relative_amp" : "unison_stereo_spread");
	    layout_set_name(label_lt, name);


	    Layout *slider_lt = layout_add_child(row_lt);
	    /* slider_lt->w.type = SCALE; */
	    slider_lt->w.type = COMPLEMENT;
	    /* slider_lt->w.value = 0.7; */
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
