#include "assets.h"
#include "color.h"
#include "dir.h"
#include "geometry.h"
#include "layout.h"
#include "modal.h"
#include "page.h"
#include "project.h"
#include "session.h"
#include "synth.h"

extern Window *main_win;

extern struct colors colors;
extern Symbol *SYMBOL_TABLE[];

static void add_osc_page(TabView *tv, Track *track);
static void add_filter_page(TabView *tv, Track *track);
static void add_amp_env_page(TabView *tv, Track *track);

void osc_bckgrnd_draw(void *arg1, void *arg2);

TabView *synth_tabview_create(Track *track)
{
    Session *session = session_get();
    TabView *tv = tabview_create(
	"Synth settings",
	session->gui.layout,
	main_win);

    if (!track->synth) track->synth = synth_create(track);
    
    track->midi_out = track->synth;
    track->midi_out_type = MIDI_OUT_SYNTH;
    
    add_osc_page(tv, track);
    add_amp_env_page(tv, track);
    add_filter_page(tv, track);
    return tv;
}

static void page_fill_out_layout(Page *p);


static Synth *synth_glob;
static int fmod_selector_fn(Dropdown *d, void *inner_arg)
{
    int modulator_i = ((long)inner_arg >> 8) & 0xFF; /* Dropdown host */
    int carrier_i = (long)inner_arg & 0xFF; /* Dropdown target */
    OscCfg *modulator = synth_glob->base_oscs + modulator_i - 1;
    /* OscCfg *carrier = carrier_i == 0 ? NULL : synth_glob->base_oscs + carrier_i - 1; */

    int target_i = carrier_i;
    /* fprintf(stderr, "WRITING INT PAIR: %d %d\n ", modulator - synth_glob->base_oscs + 1, carrier_i); */
    
    endpoint_write(&modulator->fmod_target_ep, (Value){.int_v = target_i}, true, true, true, true);

    return 0;


    /* fprintf(stderr, "MOD: %p (raw i %d), CARR: %p (raw i %d)\n", modulator, modulator_i, carrier, carrier_i); */
    /* int ret = synth_set_freq_mod_pair(synth_glob, carrier, modulator); */
    /* fprintf(stderr, "Set fmod_dropdown_reset to %d\n", modulator->fmod_dropdown_reset); */
    /* return ret; */

}

static int amod_selector_fn(Dropdown *d, void *inner_arg)
{
    int modulator_i = ((long)inner_arg >> 8) & 0xFF; /* Dropdown host */
    int carrier_i = (long)inner_arg & 0xFF; /* Dropdown target */
    OscCfg *modulator = synth_glob->base_oscs + modulator_i - 1;
    /* OscCfg *carrier = carrier_i == 0 ? NULL : synth_glob->base_oscs + carrier_i - 1; */

    int target_i = carrier_i;
    /* fprintf(stderr, "WRITING INT PAIR: %d %d\n ", modulator - synth_glob->base_oscs + 1, carrier_i); */
    
    endpoint_write(&modulator->amod_target_ep, (Value){.int_v = target_i}, true, true, true, true);

    return 0;


    /* fprintf(stderr, "MOD: %p (raw i %d), CARR: %p (raw i %d)\n", modulator, modulator_i, carrier, carrier_i); */
    /* int ret = synth_set_amp_mod_pair(synth_glob, carrier, modulator); */
    /* fprintf(stderr, "Set fmod_dropdown_reset to %d\n", modulator->fmod_dropdown_reset); */
    /* return ret; */

}


/* static int amod_selector_fn(Dropdown *d, void *inner_arg) */
/* { */
/*     int modulator_i = ((long)inner_arg >> 8) & 0xFF; /\* Dropdown host *\/ */
/*     int carrier_i = (long)inner_arg & 0xFF; /\* Dropdown target *\/ */
/*     OscCfg *modulator = synth_glob->base_oscs + modulator_i - 1; */
/*     OscCfg *carrier = carrier_i == 0 ? NULL : synth_glob->base_oscs + carrier_i - 1; */
/*     fprintf(stderr, "MOD: %p (raw i %d), CARR: %p (raw i %d)\n", modulator, modulator_i, carrier, carrier_i); */
/*     int ret = synth_set_amp_mod_pair(synth_glob, carrier, modulator); */
/*     if (modulator_i < carrier_i) { */
/* 	modulator->amod_dropdown_reset = carrier_i - 1; */
/*     } else { */
/* 	modulator->amod_dropdown_reset = carrier_i; */
/*     } */
/*     fprintf(stderr, "Set amod_dropdown_reset to %d\n", modulator->amod_dropdown_reset); */
/*     return ret; */

/* } */


static void panel_draw(void *layout_v, void *color_v)
{
    Layout *layout = layout_v;
    SDL_Color *color = color_v;
    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(color));
    geom_fill_rounded_rect(main_win->rend, &layout->rect, 8 * main_win->dpi_scale_factor);
    /* SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 255); */
    /* geom_draw_rounded_rect(main_win->rend, &layout->rect, 8 * main_win->dpi_scale_factor); */
}

static void add_osc_page(TabView *tv, Track *track)
{
    
    Synth *s = track->synth;
    synth_glob = s;
    static SDL_Color osc_bckgrnd = {40, 40, 40, 255};
    Page *page = tabview_add_page(tv, "Oscillators", SYNTH_OSCS_LT_PATH, &osc_bckgrnd, &colors.white, main_win);

    s->osc_page = page;
    /* Fill out layout */
    page_fill_out_layout(page);

    static SDL_Color sel_color = {100, 255, 150, 100};
    union page_el_params p;
    PageEl *el = NULL;

    static const SDL_Color osc_panel_colors[] = {
	{54, 34, 43, 120},
	{29, 36, 67, 120},
	/* {49, 49, 49, 255}, */
	{25, 64, 33, 120},
	{83, 54, 44, 120}
    };
    p.canvas_p.draw_fn = panel_draw;

    Layout *osc1_panel = layout_get_child_by_name_recursive(page->layout, "osc1_panel");
    p.canvas_p.draw_arg1 = osc1_panel;
    p.canvas_p.draw_arg2 = (void *)osc_panel_colors;
    page_add_el(page,EL_CANVAS,p,"","osc1_panel");

    p.canvas_p.draw_arg1 = layout_get_child_by_name_recursive(page->layout, "osc2_panel");
    p.canvas_p.draw_arg2 = (void *)(osc_panel_colors + 1);
    page_add_el(page,EL_CANVAS,p,"","osc2_panel");

    p.canvas_p.draw_arg1 = layout_get_child_by_name_recursive(page->layout, "osc3_panel");
    p.canvas_p.draw_arg2 = (void *)(osc_panel_colors + 2);
    page_add_el(page,EL_CANVAS,p,"","osc3_panel");

    p.canvas_p.draw_arg1 = layout_get_child_by_name_recursive(page->layout, "osc4_panel");
    p.canvas_p.draw_arg2 = (void *)(osc_panel_colors + 3);
    page_add_el(page,EL_CANVAS,p,"","osc4_panel");

    /* p.canvas_p.draw_arg1 = layout_get_child_by_name_recursive(osc1_panel, "1tune_panel"); */
    /* p.canvas_p.draw_arg2 = (void *)(osc_panel_colors + 3); */
    /* page_add_el(page,EL_CANVAS,p,"","1tune_panel"); */
    


    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_FILL;
    page_el_params_slider_from_ep(&p, &s->vol_ep);
    page_add_el(page,EL_SLIDER,p,"master_amp_slider","master_amp_slider");

    p.slider_p.style = SLIDER_TICK;
    page_el_params_slider_from_ep(&p, &s->pan_ep);
    page_add_el(page,EL_SLIDER,p,"master_pan_slider","master_pan_slider");

    
    
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

    p.textbox_p.text_size = 12;
    p.textbox_p.set_str = "Mod freq of:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1freq_mod_label");
        layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","2freq_mod_label");
        layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","3freq_mod_label");
        layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","4freq_mod_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    layout_reset(page->layout);

    p.textbox_p.set_str = "Mod amp of:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1amp_mod_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","2amp_mod_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","3amp_mod_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","4amp_mod_label");
    layout_size_to_fit_children_h(el->layout, true, 0);

    p.textbox_p.set_str = "Is carrier:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1fmod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","2fmod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","3fmod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","4fmod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);

    el = page_add_el(page,EL_TEXTBOX,p,"","1amod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","2amod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","3amod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","4amod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);

    layout_reset(page->layout);


    /* page_el_params_slider_from_ep(&p, &s->vol_ep); */

    /* page_add_el(page,EL_SLIDER,p,"synth_vol_slider","master_amp_slider"); */

    /* page_el_params_slider_from_ep(&p, &s->pan_ep); */
    /* page_add_el(page,EL_SLIDER,p,"synth_pan_slider","master_pan_slider"); */
    /* for (int i=0; i<SYNTH_NUM_BASE_OSCS; i++) { */
    OscCfg *cfg = s->base_oscs;

    p.sradio_p.align_horizontal = true;
    p.sradio_p.symbols = SYMBOL_TABLE + SYMBOL_SINE;
    p.sradio_p.num_items = 4;
    p.sradio_p.padding = 7;
    p.sradio_p.unsel_color = &colors.clear;
    p.sradio_p.sel_color = &sel_color;
    p.sradio_p.ep = &cfg->type_ep;
    page_add_el(page,EL_SYMBOL_RADIO,p, "1type_radio","1type_radio");

    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_FILL;
	
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

    char *osc_names[] = {"(none)", "Osc2", "Osc3", "Osc4"};
    void *dropdown_args[4];
    for (int i=0; i<4; i++) {
	dropdown_args[i] = (void *)((long)1 << 8);
    }
    dropdown_args[0] += 0; /* "none" */
    dropdown_args[1] += 2; /* osc1 */
    dropdown_args[2] += 3; /* osc2 */
    dropdown_args[3] += 4; /* osc3 */
    p.dropdown_p.header = NULL;
    p.dropdown_p.item_names = osc_names;
    p.dropdown_p.item_annotations = NULL;
    p.dropdown_p.item_args = dropdown_args;
    p.dropdown_p.num_items = 4;
    p.dropdown_p.reset_from = &cfg->fmod_dropdown_reset;
    p.dropdown_p.selection_fn = fmod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"1fmod_dropdown","1fmod_dropdown");
    
    p.dropdown_p.reset_from = &cfg->amod_dropdown_reset;
    p.dropdown_p.selection_fn = amod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"1amod_dropdown","1amod_dropdown");

    p.slight_p.value = &cfg->freq_mod_by;
    p.slight_p.val_size = sizeof(void *);
    page_add_el(page,EL_STATUS_LIGHT,p,"1fmod_status_light","1fmod_status_light");

    p.slight_p.value = &cfg->amp_mod_by;
    page_add_el(page,EL_STATUS_LIGHT,p,"1amod_status_light","1amod_status_light");

    cfg++;

    p.sradio_p.align_horizontal = true;
    p.sradio_p.symbols = SYMBOL_TABLE + SYMBOL_SINE;
    p.sradio_p.num_items = 4;
    p.sradio_p.padding = 7;
    p.sradio_p.unsel_color = &colors.clear;
    p.sradio_p.sel_color = &sel_color;
    p.sradio_p.ep = &cfg->type_ep;
    page_add_el(page,EL_SYMBOL_RADIO,p, "2type_radio","2type_radio");

    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_FILL;

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


    osc_names[1] = "Osc1";
    for (int i=0; i<4; i++) {
	dropdown_args[i] = (void *)((long)2 << 8);
    }
    dropdown_args[0] += 0; /* "none" */
    dropdown_args[1] += 1; /* osc1 */
    dropdown_args[2] += 3; /* osc2 */
    dropdown_args[3] += 4; /* osc3 */
    p.dropdown_p.header = NULL;
    p.dropdown_p.item_names = osc_names;
    p.dropdown_p.item_annotations = NULL;
    p.dropdown_p.item_args = dropdown_args;
    p.dropdown_p.num_items = 4;
    p.dropdown_p.reset_from = &cfg->fmod_dropdown_reset;
    p.dropdown_p.selection_fn = fmod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"2fmod_dropdown","2fmod_dropdown");

    p.dropdown_p.reset_from = &cfg->amod_dropdown_reset;
    p.dropdown_p.selection_fn = amod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"2amod_dropdown","2amod_dropdown");

    p.slight_p.value = &cfg->freq_mod_by;
    p.slight_p.val_size = sizeof(void *);
    page_add_el(page,EL_STATUS_LIGHT,p,"2fmod_status_light","2fmod_status_light");

    p.slight_p.value = &cfg->amp_mod_by;
    page_add_el(page,EL_STATUS_LIGHT,p,"2amod_status_light","2amod_status_light");



    cfg++;

    p.sradio_p.align_horizontal = true;
    p.sradio_p.symbols = SYMBOL_TABLE + SYMBOL_SINE;
    p.sradio_p.num_items = 4;
    p.sradio_p.padding = 7;
    p.sradio_p.unsel_color = &colors.clear;
    p.sradio_p.sel_color = &sel_color;
    p.sradio_p.ep = &cfg->type_ep;
    page_add_el(page,EL_SYMBOL_RADIO,p, "3type_radio","3type_radio");
    
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_FILL;

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

    
    osc_names[2] = "Osc2";
    for (int i=0; i<4; i++) {
	dropdown_args[i] = (void *)((long)3 << 8);
    }
    dropdown_args[0] += 0; /* "none" */
    dropdown_args[1] += 1; /* osc1 */
    dropdown_args[2] += 2; /* osc2 */
    dropdown_args[3] += 4; /* osc3 */
    p.dropdown_p.header = NULL;
    p.dropdown_p.item_names = osc_names;
    p.dropdown_p.item_annotations = NULL;
    p.dropdown_p.item_args = dropdown_args;
    p.dropdown_p.num_items = 4;
    p.dropdown_p.reset_from = &cfg->fmod_dropdown_reset;
    p.dropdown_p.selection_fn = fmod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"3fmod_dropdown","3fmod_dropdown");

    p.dropdown_p.reset_from = &cfg->amod_dropdown_reset;
    p.dropdown_p.selection_fn = amod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"3amod_dropdown","3amod_dropdown");


    p.slight_p.value = &cfg->freq_mod_by;
    p.slight_p.val_size = sizeof(void *);
    page_add_el(page,EL_STATUS_LIGHT,p,"3fmod_status_light","3fmod_status_light");

    p.slight_p.value = &cfg->amp_mod_by;
    page_add_el(page,EL_STATUS_LIGHT,p,"3amod_status_light","3amod_status_light");



    cfg++;

    p.sradio_p.align_horizontal = true;
    p.sradio_p.symbols = SYMBOL_TABLE + SYMBOL_SINE;
    p.sradio_p.num_items = 4;
    p.sradio_p.padding = 7;
    p.sradio_p.unsel_color = &colors.clear;
    p.sradio_p.sel_color = &sel_color;
    p.sradio_p.ep = &cfg->type_ep;
    page_add_el(page,EL_SYMBOL_RADIO,p, "4type_radio","4type_radio");


    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_FILL;

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



    /* p.sradio_p.align_horizontal = true; */
    /* p.sradio_p.symbols = SYMBOL_TABLE + SYMBOL_SINE; */
    /* p.sradio_p.num_items = 4; */
    /* p.sradio_p.padding = 7; */
    /* p.sradio_p.unsel_color = &colors.clear; */
    /* p.sradio_p.sel_color = &sel_color; */
    /* p.sradio_p.ep = &cfg->type_ep; */
    /* page_add_el(page,EL_SYMBOL_RADIO,p,"4type_radio","4type_radio"); */
    /* /\* } *\/ */

  
    osc_names[3] = "Osc3";
    for (int i=0; i<4; i++) {
	dropdown_args[i] = (void *)((long)4 << 8);
    }
    dropdown_args[0] += 0; /* "none" */
    dropdown_args[1] += 1; /* osc1 */
    dropdown_args[2] += 2; /* osc2 */
    dropdown_args[3] += 3; /* osc3 */
    p.dropdown_p.header = NULL;
    p.dropdown_p.item_names = osc_names;
    p.dropdown_p.item_annotations = NULL;
    p.dropdown_p.item_args = dropdown_args;
    p.dropdown_p.num_items = 4;
    p.dropdown_p.reset_from = &cfg->fmod_dropdown_reset;
    p.dropdown_p.selection_fn = fmod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"4fmod_dropdown","4fmod_dropdown");

    p.dropdown_p.reset_from = &cfg->amod_dropdown_reset;
    p.dropdown_p.selection_fn = amod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"4amod_dropdown","4amod_dropdown");

    p.slight_p.value = &cfg->freq_mod_by;
    p.slight_p.val_size = sizeof(void *);
    page_add_el(page,EL_STATUS_LIGHT,p,"4fmod_status_light","4fmod_status_light");

    p.slight_p.value = &cfg->amp_mod_by;
    page_add_el(page,EL_STATUS_LIGHT,p,"4amod_status_light","4amod_status_light");


    
    page_reset(page);
	
}

static void add_amp_env_page(TabView *tv, Track *track)
{
    Synth *s = track->synth;
    static SDL_Color amp_bckgrnd = {30, 50, 30, 255};
    Page *page = tabview_add_page(tv, "Amp Envelope", SYNTH_AMP_ENV_LT_PATH, &amp_bckgrnd, &colors.white, main_win);

    s->amp_env_page = page;
    
    PageElParams p;
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 16;
    p.textbox_p.win = main_win;

    p.textbox_p.set_str = "Attack:";
    page_add_el(page,EL_TEXTBOX,p,"","attack_label");

    p.textbox_p.set_str = "Decay:";
    page_add_el(page,EL_TEXTBOX,p,"","decay_label");

    p.textbox_p.set_str = "Sustain:";
    page_add_el(page,EL_TEXTBOX,p,"","sustain_label");

    p.textbox_p.set_str = "Release:";
    page_add_el(page,EL_TEXTBOX,p,"","release_label");

    p.textbox_p.set_str = "Ramp exponent:";
    page_add_el(page,EL_TEXTBOX,p,"","ramp_exp_label");

    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_TICK;

    page_el_params_slider_from_ep(&p, &s->amp_env.a_ep);	
    page_add_el(page,EL_SLIDER,p,"attack_slider","attack_slider");

    page_el_params_slider_from_ep(&p, &s->amp_env.d_ep);	
    page_add_el(page,EL_SLIDER,p,"decay_slider","decay_slider");

    p.slider_p.style = SLIDER_FILL;
    page_el_params_slider_from_ep(&p, &s->amp_env.s_ep);	
    page_add_el(page,EL_SLIDER,p,"sustain_slider","sustain_slider");

    p.slider_p.style = SLIDER_TICK;
    page_el_params_slider_from_ep(&p, &s->amp_env.r_ep);	
    page_add_el(page,EL_SLIDER,p,"release_slider","release_slider");

    page_el_params_slider_from_ep(&p, &s->amp_env.ramp_exp_ep);	
    page_add_el(page,EL_SLIDER,p,"ramp_exp_slider","ramp_exp_slider");

    page_reset(page);

}


static void add_filter_page(TabView *tv, Track *track)
{

    Synth *s = track->synth;
    static SDL_Color filter_bckgrnd = {60, 30, 20, 255};
    Page *page = tabview_add_page(tv, "Filter", SYNTH_FILTER_LT_PATH, &filter_bckgrnd, &colors.white, main_win);

    s->filter_page = page;

    PageElParams p;

    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 16;
    p.textbox_p.win = main_win;

    p.textbox_p.set_str = "Active:";
    page_add_el(page,EL_TEXTBOX,p,"","active_label");

    p.textbox_p.set_str = "Freq scalar:";
    page_add_el(page,EL_TEXTBOX,p,"","freq_scalar_label");

    p.textbox_p.set_str = "Velocity freq scalar:";
    page_add_el(page,EL_TEXTBOX,p,"","velocity_freq_scalar_label");

    p.textbox_p.set_str = "Resonance:";
    page_add_el(page,EL_TEXTBOX,p,"","resonance_label");

    p.textbox_p.set_str = "Use amp env:";
    page_add_el(page,EL_TEXTBOX,p,"","use_amp_env_label");

    p.textbox_p.set_str = "Attack:";
    page_add_el(page,EL_TEXTBOX,p,"","attack_label");

    p.textbox_p.set_str = "Decay:";
    page_add_el(page,EL_TEXTBOX,p,"","decay_label");

    p.textbox_p.set_str = "Sustain:";
    page_add_el(page,EL_TEXTBOX,p,"","sustain_label");

    p.textbox_p.set_str = "Release:";
    page_add_el(page,EL_TEXTBOX,p,"","release_label");

    p.textbox_p.set_str = "Ramp exponent:";
    page_add_el(page,EL_TEXTBOX,p,"","ramp_exp_label");


    p.toggle_p.ep = &s->filter_active_ep;
    page_add_el(page,EL_TOGGLE,p,"filter_active_toggle","active_toggle");

    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_FILL;

    page_el_params_slider_from_ep(&p, &s->freq_scalar_ep);
    page_add_el(page,EL_SLIDER,p,"freq_scalar_slider","freq_scalar_slider");

    page_el_params_slider_from_ep(&p, &s->velocity_freq_scalar_ep);
    page_add_el(page,EL_SLIDER,p,"velocity_freq_scalar_slider","velocity_freq_scalar_slider");

    page_el_params_slider_from_ep(&p, &s->resonance_ep);
    page_add_el(page,EL_SLIDER,p,"resonance_slider","resonance_slider");

    p.toggle_p.ep = &s->use_amp_env_ep;
    page_add_el(page,EL_TOGGLE,p,"use_amp_env_toggle","use_amp_env_toggle");



    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_TICK;

    page_el_params_slider_from_ep(&p, &s->filter_env.a_ep);	
    page_add_el(page,EL_SLIDER,p,"attack_slider","attack_slider");

    page_el_params_slider_from_ep(&p, &s->filter_env.d_ep);	
    page_add_el(page,EL_SLIDER,p,"decay_slider","decay_slider");

    p.slider_p.style = SLIDER_FILL;
    page_el_params_slider_from_ep(&p, &s->filter_env.s_ep);	
    page_add_el(page,EL_SLIDER,p,"sustain_slider","sustain_slider");

    p.slider_p.style = SLIDER_TICK;
    page_el_params_slider_from_ep(&p, &s->filter_env.r_ep);	
    page_add_el(page,EL_SLIDER,p,"release_slider","release_slider");

    page_el_params_slider_from_ep(&p, &s->filter_env.ramp_exp_ep);	
    page_add_el(page,EL_SLIDER,p,"ramp_exp_slider","ramp_exp_slider");




    page_reset(page);

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
	Layout *freq_mod_row = layout_get_child_by_name_recursive(page->layout, "freq_mod_row");
	Layout *amp_mod_row = layout_get_child_by_name_recursive(page->layout, "amp_mod_row");
	snprintf(name, 255, "%dfreq_mod_row", o+1);
	layout_set_name(freq_mod_row, name);
	snprintf(name, 255, "%damp_mod_row", o+1);
	layout_set_name(amp_mod_row, name);
	Layout *fmod_label_lt = layout_add_child(freq_mod_row);
	Layout *amod_label_lt = layout_add_child(amp_mod_row);
	snprintf(name, 255, "%dfreq_mod_label", o+1);
	layout_set_name(fmod_label_lt, name);
	snprintf(name, 255, "%damp_mod_label", o+1);
	layout_set_name(amod_label_lt, name);
	fmod_label_lt->h.type = SCALE;
	fmod_label_lt->h.value = 1.0;
	fmod_label_lt->w.value = 20.0;
	amod_label_lt->h.type = SCALE;
	amod_label_lt->h.value = 1.0;
	amod_label_lt->w.value = 20.0;

	Layout *fmod_dropdown = layout_add_child(freq_mod_row);
	snprintf(name, 255, "%dfmod_dropdown", o+1);
	layout_set_name(fmod_dropdown, name);
	fmod_dropdown->h.type = SCALE;
	fmod_dropdown->h.value = 1.0;
	fmod_dropdown->x.type = STACK;
	fmod_dropdown->x.value = 5.0;
	fmod_dropdown->w.value = 80;

	Layout *amod_dropdown = layout_copy(fmod_dropdown, amp_mod_row);
	snprintf(name, 255, "%damod_dropdown", o+1);
	layout_set_name(amod_dropdown, name);

	

	Layout *fmod_status_light_label = layout_add_child(freq_mod_row);
	snprintf(name, 255, "%dfmod_status_light_label", o+1);
	layout_set_name(fmod_status_light_label, name);
	fmod_status_light_label->h.type = SCALE;
	fmod_status_light_label->h.value = 1.0;
	fmod_status_light_label->x.type = STACK;
	fmod_status_light_label->x.value = 20.0;
	fmod_status_light_label->w.value = 50;

	Layout *amod_status_light_label = layout_copy(fmod_status_light_label, amp_mod_row);
	snprintf(name, 255, "%damod_status_light_label", o+1);
	layout_set_name(amod_status_light_label, name);


	Layout *fmod_status_light = layout_add_child(freq_mod_row);
	snprintf(name, 255, "%dfmod_status_light", o+1);
	layout_set_name(fmod_status_light, name);
	fmod_status_light->h.type = SCALE;
	fmod_status_light->h.value = 1.0;
	fmod_status_light->x.type = STACK;
	fmod_status_light->x.value = 1.0;
	fmod_status_light->w.value = 20;

	Layout *amod_status_light = layout_copy(fmod_status_light, amp_mod_row);
	snprintf(name, 255, "%damod_status_light", o+1);
	layout_set_name(amod_status_light, name);	
	
    }

    layout_force_reset(page->layout);
    /* FILE *f = fopen("_synth_oscs.xml", "w"); */
    /* layout_write(f, page->layout, 0); */
    /* fclose(f); */
    /* exit(0); */
}

static int dir_to_tline_filter_synth(void *dp_v, void *dn_v)
{
    DirPath *dp = *(DirPath **)dp_v;
    if (dp->hidden) {
	return 0;
    }

    if (dp->type != DT_DIR) {
	char *dotpos = strrchr(dp->path, '.');
	if (!dotpos) {
	    return 0;
	}
	char *ext = dotpos + 1;
	if (strncmp("jsynth", ext, 6) *
	    strncmp("JSYNTH", ext, 6) == 0) {
	    return 1;
	}
	return 0;
    } else {
	return 1;
    }

}

static void synth_open_form(DirNav *dn, DirPath *dp)
{
    Session *session = session_get();
    char *dotpos = strrchr(dp->path, '.');
    if (!dotpos) {
	status_set_errstr("Cannot open file without a .jsynth extension");
	fprintf(stderr, "Cannot open file without a .jsynth extension\n");
	return;
    }
    char *ext = dotpos + 1;
    /* fprintf(stdout, "ext char : %c\n", *ext); */
    if (strcmp("jsynth", ext) * strcmp("JSYNTH", ext) == 0) {
	fprintf(stdout, "Wav file selected\n");
	Timeline *tl = ACTIVE_TL;
	Track *track = timeline_selected_track(tl);
	if (!track) return;
	if (!track->synth) return;
	synth_read_preset_file(dp->path, track->synth);
    } else {
	status_set_errstr("Cannot open file without a .jsynth extension");
	fprintf(stderr, "Cannot open file without a .jsynth extension\n");
	return;

    }


}

/* static int synth_open_form(void *mod_v, void *target) */
/* { */
/*     fprintf(stderr, "Opening form\n"); */
/*     Session *session = session_get(); */
/*     Modal *modal = mod_v; */
/*     char *dirpath = NULL; */
/*     for (int i=0; i<modal->num_els; i++) { */
/* 	fprintf(stderr, "\tchecking %d/%d\n", i,  modal->num_els); */

/* 	ModalEl *el = modal->els[i]; */
/* 	switch(el->type) { */
/* 	case MODAL_EL_DIRNAV: */
/* 	    fprintf(stderr, "Found dirnav!\n"); */
/* 	    dirpath = ((DirNav *)el->obj)->current_path_tb->text->value_handle; */
/* 	    break; */
/* 	    break; */
/* 	default: */
/* 	    break; */
/* 	} */
/*     } */
/*     if (dirpath) { */
/* 	Timeline *tl = ACTIVE_TL; */
/* 	Track *track = timeline_selected_track(tl); */
/* 	fprintf(stderr, "Selected track %p synth: %p\n", track, track->synth); */
/* 	if (track && track->synth) { */
/* 	    synth_read_preset_file(dirpath, track->synth); */
/* 	} */
/* 	window_pop_modal(main_win); */
/* 	return 0; */
/*     } else { */
/* 	return -1; */
/*     } */
/* } */

static int synth_save_form(void *mod_v, void *target)
{
    char full_path[MAX_NAMELENGTH];
    Session *session = session_get();
    Modal *modal = mod_v;
    char *dirpath = NULL;
    char *name = NULL;
    for (int i=0; i<modal->num_els; i++) {

	ModalEl *el = modal->els[i];
	switch(el->type) {
	case MODAL_EL_TEXTENTRY:
	    name = ((TextEntry *)el->obj)->tb->text->display_value;
	    break;

	case MODAL_EL_DIRNAV:
	    dirpath = ((DirNav *)el->obj)->current_path_tb->text->value_handle;
	    break;
	default:
	    break;
	}
    }
    if (dirpath && name) {
	Timeline *tl = ACTIVE_TL;
	Track *track = timeline_selected_track(tl);
	if (track && track->synth) {
	    int offset = snprintf(full_path, MAX_NAMELENGTH, "%s", dirpath);
	    full_path[offset] = '/';
	    offset++;
	    offset += snprintf(full_path + offset, MAX_NAMELENGTH - offset, "%s", name);
	    fprintf(stderr, "NAME before call? %s\n", name);
	    synth_write_preset_file(full_path, track->synth);
	}
	window_pop_modal(main_win);
	return 0;
    } else {
	return -1;
    }
}




/* Top-level function to create modal */
void synth_open_preset()
{
    Layout *layout = layout_add_child(main_win->layout);
    layout_set_default_dims(layout);
    
    Modal *modal = modal_create(layout);
    modal_add_header(modal, "Open synth preset", &colors.light_grey, 3);
    ModalEl *el = modal_add_dirnav(modal, ".", dir_to_tline_filter_synth);
    DirNav *dn = el->obj;
    dn->file_select_action = synth_open_form;
    /* modal_add_button(modal, "Open", synth_open_form); */
    /* modal->submit_form = synth_open_form; */
    window_push_modal(main_win, modal);
    modal_reset(modal);
    /* fprintf(stdout, "about to call move onto\n"); */
    modal_move_onto(modal);

    
}

static int file_ext_completion_jsynth(Text *txt, void *obj)
{
    char *dotpos = strrchr(txt->display_value, '.');
    int retval = 0;
    /* fprintf(stdout, "COMPLETION %s\n", dotpos); */
    if (!dotpos) {
	strcat(txt->display_value, ".jsynth");
	txt->len = strlen(txt->display_value);
	txt->cursor_end_pos = txt->len;
	txt_reset_drawable(txt);
	retval = 0;
    } else if (strcmp(dotpos, ".jsynth") != 0 && (strcmp(dotpos, ".JSYTNH") != 0)) {
	retval = 1;
    }
    if (retval == 1) {
	txt->cursor_start_pos = dotpos + 1 - txt->display_value;
	txt->cursor_end_pos = txt->len;
	status_set_errstr("Export file must have \".jsynth\" extension");
    }
    return retval;
}

void synth_save_preset()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track || !track->synth) {
	return;
    }
    Layout *layout = layout_add_child(main_win->layout);
    layout_set_default_dims(layout);
    
    Modal *modal = modal_create(layout);
    modal_add_header(modal, "Save synth preset as:", &colors.light_grey, 3);
    modal_add_textentry(
	modal,
	track->synth->preset_name,
	MAX_NAMELENGTH,
	txt_name_validation,
	file_ext_completion_jsynth,
	NULL);
    modal_add_header(modal, "File location:", &colors.light_grey, 5);
    modal_add_dirnav(modal, ".", dir_to_tline_filter_synth);
    modal_add_button(modal, "Save", synth_save_form);
    modal->submit_form = synth_save_form;
    window_push_modal(main_win, modal);
    modal_reset(modal);
    /* fprintf(stdout, "about to call move onto\n"); */
    modal_move_onto(modal);
}

