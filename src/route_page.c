#include "color.h"
#include "endpoint_callbacks.h"
#include "layout.h"
#include "page.h"
#include "route.h"
#include "session.h"
#include "symbol.h"

extern Window *main_win;
extern struct colors colors;

PageListItemFnDef(create_route_out_page)
{
    AudioRoute **item_loc = item;
    AudioRoute *route = *item_loc;
    PageElParams p;

    page_el_params_slider_from_ep(&p, &route->amp_ep);
    p.slider_p.style = SLIDER_FILL;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    page_add_el(page, EL_SLIDER, p, "", "amp_s");
    
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 18;
    p.textbox_p.win = main_win;
    p.textbox_p.set_str = "→";
    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "", "arw");
    textbox_style(
	el->component,
	CENTER,
	false,
	NULL,
	&colors.white);

    p.textbox_p.text_size = 14;
    p.textbox_p.set_str = route->dst->name;
    el = page_add_el(page, EL_TEXTBOX, p, "", "dst");
    textbox_style(
	el->component,
	CENTER,
	true,
	&route->dst->color,
	&colors.white);
    textbox_reset_full(el->component);
    /* textbox_size_to_fit_h(el->component, 8); */

    p.sbutton_p.action = NULL;
    p.sbutton_p.symbol_index = SYMBOL_X;
    p.sbutton_p.target = NULL;
    /* static const SDL_Color pale_red =  */
    p.sbutton_p.background_color = &colors.x_red;
    el = page_add_el(page, EL_SYMBOL_BUTTON, p, "", "del");
    
    SymbolButton *sb = el->component;
    sb->layout->w.value = SYMBOL_STD_DIM;
    sb->layout->h.value = SYMBOL_STD_DIM;
    layout_reset(sb->layout);
    layout_center_agnostic(sb->layout, false, true);
};

PageListItemFnDef(create_route_in_page)
{
    AudioRoute **item_loc = item;
    AudioRoute *route = *item_loc;
    PageElParams p;

    page_el_params_slider_from_ep(&p, &route->amp_ep);
    p.slider_p.style = SLIDER_FILL;
    p.slider_p.orientation = SLIDER_HORIZONTAL;
    page_add_el(page, EL_SLIDER, p, "", "amp_s");
    
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 18;
    p.textbox_p.win = main_win;
    p.textbox_p.set_str = "→";
    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "", "arw1");
    textbox_style(
	el->component,
	CENTER,
	false,
	NULL,
	&colors.white);

    p.textbox_p.set_str = "→ •";
    el = page_add_el(page, EL_TEXTBOX, p, "", "arw2");
    textbox_style(
	el->component,
	CENTER,
	false,
	NULL,
	&colors.white);


    p.textbox_p.text_size = 14;
    p.textbox_p.set_str = route->src->name;
    el = page_add_el(page, EL_TEXTBOX, p, "", "src");
    textbox_style(
	el->component,
	CENTER,
	true,
	&route->src->color,
	&colors.white);
    textbox_reset_full(el->component);
    /* textbox_size_to_fit_h(el->component, 8); */

    p.sbutton_p.action = NULL;
    p.sbutton_p.symbol_index = SYMBOL_X;
    p.sbutton_p.target = NULL;
    /* static const SDL_Color pale_red =  */
    p.sbutton_p.background_color = &colors.x_red;
    el = page_add_el(page, EL_SYMBOL_BUTTON, p, "", "del");
    
    SymbolButton *sb = el->component;
    sb->layout->w.value = SYMBOL_STD_DIM;
    sb->layout->h.value = SYMBOL_STD_DIM;
    layout_reset(sb->layout);
    layout_center_agnostic(sb->layout, false, true);
};


static const SDL_Color routes_in_color = {107, 46, 43, 255};
static const SDL_Color routes_out_color = {15, 82, 64, 255};

static void create_routes_tabview(Track *track)
{

}

static void create_ins_page(TabView *tv, Track *track)
{
    Page *page = tabview_add_page(
	tv,
	"In",
	AUDIO_ROUTES_LT_PATH,
	&routes_in_color,
	&colors.white,
	main_win);

    
    PageElParams p;    
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.win = main_win;

    static char routes_title[MAX_NAMELENGTH + 8];
    snprintf(routes_title, MAX_NAMELENGTH + 8, "%s routes IN\n", track->name);
    p.textbox_p.text_size = 16;
    p.textbox_p.set_str = routes_title;
    page_add_el(page, EL_TEXTBOX, p, "", "title");

    p.page_list_p.item_size = sizeof(AudioRoute *);
    p.page_list_p.item_template_filepath = AUDIO_ROUTE_IN_TEMPLATE_LT_PATH;
    p.page_list_p.num_items = track->num_route_ins;
    p.page_list_p.items_loc = track->route_ins;
    p.page_list_p.create_item_page_fn = create_route_in_page;
    page_add_el(page, EL_PAGE_LIST, p, "", "rts");


}
static void create_outs_page(TabView *tv, Track *track)
{
    Page *page = tabview_add_page(
	tv,
	"Out",
	AUDIO_ROUTES_LT_PATH,
	&routes_out_color,
	&colors.white,
	main_win);

    PageElParams p;    
    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.win = main_win;

    static char routes_title[MAX_NAMELENGTH + 8];
    snprintf(routes_title, MAX_NAMELENGTH + 8, "%s routes OUT\n", track->name);
    p.textbox_p.text_size = 16;
    p.textbox_p.set_str = routes_title;
    page_add_el(page, EL_TEXTBOX, p, "", "title");
    
    p.textbox_p.text_size = 14;
    p.textbox_p.set_str = "Send to main out:";    
    page_add_el(page, EL_TEXTBOX, p, "", "mro_lbl");

    p.toggle_p.ep = &track->send_to_out_ep;
    page_add_el(page, EL_TOGGLE, p, "mro_tgl", "mro_tgl");

    p.page_list_p.item_size = sizeof(AudioRoute *);
    p.page_list_p.item_template_filepath = AUDIO_ROUTE_OUT_TEMPLATE_LT_PATH;
    p.page_list_p.num_items = track->num_routes;
    p.page_list_p.items_loc = track->routes;
    p.page_list_p.create_item_page_fn = create_route_out_page;
    page_add_el(page, EL_PAGE_LIST, p, "", "rts");

    p.sbutton_p.action = NULL;
    p.sbutton_p.background_color = &colors.play_green;
    p.sbutton_p.symbol_index = SYMBOL_PLUS;
    p.sbutton_p.target = NULL;
    page_add_el(page, EL_SYMBOL_BUTTON, p, "", "pls");

}

void route_page_open(Track *track)
{
    Session *session = session_get();
    TabView *tv = tabview_create("Track audio routes", session->gui.layout, main_win);
    create_ins_page(tv, track);
    create_outs_page(tv, track);

    
    tabview_activate(tv, track, track->name);
    track->tl->needs_redraw = true;    
}


