#include "color.h"
#include "endpoint_callbacks.h"
#include "layout.h"
#include "modal.h"
#include "page.h"
#include "route.h"
#include "session.h"
#include "symbol.h"
#include "window.h"

extern Window *main_win;
extern struct colors colors;

struct add_route_arg {
    Track *src;
    Track *dst;
    PageList *list_to_update;
};

static void add_route_buttonfn_internal(Track *trck, bool from_dst, Modal *modal)
{
    Dropdown *dd = NULL;
    for (int i=0; i<modal->num_els; i++) {
	if (modal->els[i]->type == MODAL_EL_DROPDOWN) {
	    dd = modal->els[i]->obj;
	    break;
	}
    }
    if (!dd) {
	fprintf(stderr, "Critical error: no dropdown on add route modal\n");
	exit(1);
    }
    Track *trck2 = dd->item_args[dd->selected_item];
    if (from_dst) {
	track_add_audio_route(trck2, trck, 1.0);
    } else {
	track_add_audio_route(trck, trck2, 1.0);
    }

}

static ComponentFnDef(add_route_buttonfn)
{
    add_route_buttonfn_internal(target, false, self);
    return 0;
}

static ComponentFnDef(add_route_from_dst_buttonfn)
{
    add_route_buttonfn_internal(target, true, self);
    return 0;
}


static int add_route_internal(Track *trck, bool from_dst)
{
    if (trck->tl->num_tracks == 1) {
	status_set_errstr("No candidate route destination tracks");
	return 1;
    }

    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    layout_reset(mod_lt);
    Modal *modal = modal_create(mod_lt);
    modal_add_header(modal, "Add audio route", &colors.white, 3);
    static char from_str[256];
    memset(from_str, 0, sizeof(from_str));

    if (from_dst) {
	snprintf(from_str, 256, "to %s", trck->name);
    } else {
	snprintf(from_str, 256, "from %s", trck->name);
    }
    modal_add_header(modal, from_str, &colors.white, 5);
    if (from_dst) {
	modal_add_header(modal, "from:", &colors.white, 5);
    } else {
	modal_add_header(modal, "to:", &colors.white, 5);
    }
    /* modal_add_dropdown(modal,  */
    const char **options = malloc(sizeof(char *) * trck->tl->num_tracks - 1);
    void **args = malloc(sizeof(void *) * trck->tl->num_tracks - 1);
    int j = 0;
    for (int i=0; i<trck->tl->num_tracks; i++) {
	if (trck->tl->tracks[i] == trck) continue;
	options[j] = trck->tl->tracks[i]->name;
	args[j] = trck->tl->tracks[i];
	j++;
    }
    /* static int trck_option = 0; */

    modal_add_dropdown(
	modal,
	from_dst ? "Add route from:" : "Add route to:",
	options,
	NULL,
	args,
	trck->tl->num_tracks - 1,
	NULL,
	NULL);
    modal->stashed_obj = trck;
    modal_add_button(
	modal,
	"Add",
	from_dst ? add_route_from_dst_buttonfn : add_route_buttonfn);
	
    
    window_push_modal(main_win, modal);
    modal_reset(modal);
    modal_move_onto(modal);
    return 0;
    /* track_add_audio_route(arg->src, arg->dst, 1.0); */
}

static ComponentFnDef(add_route)
{
    Track *src = target;
    return add_route_internal(src, false);
}

static ComponentFnDef(add_route_from_dst)
{
    Track *src = target;
    return add_route_internal(src, true);
  
}

ComponentFnDef(route_del_button)
{
    AudioRoute *to_delete = target;
    /* Page *page = to_delete->page; */
    audio_route_delete(to_delete);
    /* page_list_update(page->page_list, page->page_list->num_items - 1); */
    return 0;
}

PageListItemFnDef(create_route_out_page)
{
    AudioRoute **item_loc = item;
    AudioRoute *route = *item_loc;
    PageElParams p;

    page->linked_obj = route;
    page->linked_obj_type = PAGE_ROUTE;
    route->page = page;
    /* route->page_list = page->page_list; */
    /* page->page_list->connected_obj = route->dst; */
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

    p.sbutton_p.action = route_del_button;
    p.sbutton_p.symbol_index = SYMBOL_X;
    p.sbutton_p.target = route;
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
    page->linked_obj = route;
    page->linked_obj_type = PAGE_ROUTE;
    route->page = page;
    
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

    p.sbutton_p.action = route_del_button;
    p.sbutton_p.symbol_index = SYMBOL_X;
    p.sbutton_p.target = route;
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

    p.sbutton_p.action = add_route_from_dst;
    p.sbutton_p.background_color = &colors.play_green;
    p.sbutton_p.symbol_index = SYMBOL_PLUS;
    p.sbutton_p.target = track;
    page_add_el(page, EL_SYMBOL_BUTTON, p, "", "pls");

    p.page_list_p.item_size = sizeof(AudioRoute *);
    p.page_list_p.item_template_filepath = AUDIO_ROUTE_IN_TEMPLATE_LT_PATH;
    p.page_list_p.num_items = track->num_route_ins;
    p.page_list_p.items_loc = track->route_ins;
    p.page_list_p.create_item_page_fn = create_route_in_page;
    PageEl *el = page_add_el(page, EL_PAGE_LIST, p, "", "rts");

    PageList *pl = el->component;
    pl->monitor_num_items = &track->num_route_ins;
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

    p.sbutton_p.action = add_route;
    p.sbutton_p.background_color = &colors.play_green;
    p.sbutton_p.symbol_index = SYMBOL_PLUS;
    p.sbutton_p.target = track;
    page_add_el(page, EL_SYMBOL_BUTTON, p, "", "pls");

    p.toggle_p.ep = &track->send_to_out_ep;
    page_add_el(page, EL_TOGGLE, p, "mro_tgl", "mro_tgl");

    p.page_list_p.item_size = sizeof(AudioRoute *);
    p.page_list_p.item_template_filepath = AUDIO_ROUTE_OUT_TEMPLATE_LT_PATH;
    p.page_list_p.num_items = track->num_routes;
    p.page_list_p.items_loc = track->routes;
    p.page_list_p.create_item_page_fn = create_route_out_page;
    PageEl *el = page_add_el(page, EL_PAGE_LIST, p, "", "rts");
    PageList *pl = el->component;
    pl->monitor_num_items = &track->num_routes;
}

void route_page_open(Track *track)
{
    Session *session = session_get();
    TabView *tv = tabview_create("Track audio routes", session->gui.layout, main_win);
    create_ins_page(tv, track);
    create_outs_page(tv, track);
    
    tabview_activate(tv, track, track->name);
    tabview_select_tab(tv, 1);
    track->tl->needs_redraw = true;    
}


