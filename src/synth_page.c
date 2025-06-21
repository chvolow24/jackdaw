#include "assets.h"
#include "color.h"
#include "geometry.h"
#include "layout.h"
#include "page.h"
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
    OscCfg *carrier = carrier_i == 0 ? NULL : synth_glob->base_oscs + carrier_i - 1;
    fprintf(stderr, "MOD: %p (raw i %d), CARR: %p (raw i %d)\n", modulator, modulator_i, carrier, carrier_i);
    int ret = synth_set_freq_mod_pair(synth_glob, carrier, modulator);
    fprintf(stderr, "RET: %d\n", ret);
    return ret;

}

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

    p.canvas_p.draw_arg1 = layout_get_child_by_name_recursive(osc1_panel, "1tune_panel");
    p.canvas_p.draw_arg2 = (void *)(osc_panel_colors + 3);
    page_add_el(page,EL_CANVAS,p,"","1tune_panel");
    




    
    
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
    page_add_el(page,EL_TEXTBOX,p,"","1amp_mod_label");
    page_add_el(page,EL_TEXTBOX,p,"","2amp_mod_label");
    page_add_el(page,EL_TEXTBOX,p,"","3amp_mod_label");
    page_add_el(page,EL_TEXTBOX,p,"","4amp_mod_label");

    p.textbox_p.set_str = "Is carrier:";
    el = page_add_el(page,EL_TEXTBOX,p,"","1fmod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","2fmod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","3fmod_status_light_label");
    layout_size_to_fit_children_h(el->layout, true, 0);
    el = page_add_el(page,EL_TEXTBOX,p,"","4fmod_status_light_label");
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
    p.dropdown_p.selection_fn = fmod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"1fmod_dropdown","1fmod_dropdown");

    p.slight_p.value = &cfg->freq_mod_by;
    p.slight_p.val_size = sizeof(void *);
    page_add_el(page,EL_STATUS_LIGHT,p,"1fmod_status_light","1fmod_status_light");

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
    p.dropdown_p.selection_fn = fmod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"2fmod_dropdown","2fmod_dropdown");

    p.slight_p.value = &cfg->freq_mod_by;
    p.slight_p.val_size = sizeof(void *);
    page_add_el(page,EL_STATUS_LIGHT,p,"2fmod_status_light","2fmod_status_light");


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
    p.dropdown_p.selection_fn = fmod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"3fmod_dropdown","3fmod_dropdown");

    p.slight_p.value = &cfg->freq_mod_by;
    p.slight_p.val_size = sizeof(void *);
    page_add_el(page,EL_STATUS_LIGHT,p,"3fmod_status_light","3fmod_status_light");


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
    p.dropdown_p.selection_fn = fmod_selector_fn;
    page_add_el(page,EL_DROPDOWN,p,"4fmod_dropdown","4fmod_dropdown");

    p.slight_p.value = &cfg->freq_mod_by;
    p.slight_p.val_size = sizeof(void *);
    page_add_el(page,EL_STATUS_LIGHT,p,"4fmod_status_light","4fmod_status_light");

    
    layout_force_reset(page->layout);
	
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

    p.textbox_p.set_str = "Exponent:";
    page_add_el(page,EL_TEXTBOX,p,"","exponent_label");

    p.slider_p.orientation = SLIDER_HORIZONTAL;
    p.slider_p.style = SLIDER_FILL;

    page_el_params_slider_from_ep(&p, &s->amp_env.a_ep);	
    page_add_el(page,EL_SLIDER,p,"attack_slider","attack_slider");

    page_el_params_slider_from_ep(&p, &s->amp_env.d_ep);	
    page_add_el(page,EL_SLIDER,p,"decay_slider","decay_slider");

    page_el_params_slider_from_ep(&p, &s->amp_env.s_ep);	
    page_add_el(page,EL_SLIDER,p,"sustain_slider","sustain_slider");

    page_el_params_slider_from_ep(&p, &s->amp_env.r_ep);	
    page_add_el(page,EL_SLIDER,p,"release_slider","release_slider");







    

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

	Layout *fmod_status_light_label = layout_add_child(freq_mod_row);
	snprintf(name, 255, "%dfmod_status_light_label", o+1);
	layout_set_name(fmod_status_light_label, name);
	fmod_status_light_label->h.type = SCALE;
	fmod_status_light_label->h.value = 1.0;
	fmod_status_light_label->x.type = STACK;
	fmod_status_light_label->x.value = 20.0;
	fmod_status_light_label->w.value = 50;

	Layout *fmod_status_light = layout_add_child(freq_mod_row);
	snprintf(name, 255, "%dfmod_status_light", o+1);
	layout_set_name(fmod_status_light, name);
	fmod_status_light->h.type = SCALE;
	fmod_status_light->h.value = 1.0;
	fmod_status_light->x.type = STACK;
	fmod_status_light->x.value = 1.0;
	fmod_status_light->w.value = 20;

	
	
	
    }

    layout_force_reset(page->layout);
    /* FILE *f = fopen("_synth_oscs.xml", "w"); */
    /* layout_write(f, page->layout, 0); */
    /* fclose(f); */
    /* exit(0); */


}
