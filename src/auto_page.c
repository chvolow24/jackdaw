#include "auto_page.h"


#include "effect_pages.h"

extern Window *main_win;

/* Auto-page: automatically create a page (like an effect page)
   from a list of params */


void auto_page(Page *page, AutoPageEl *els, int num_els)
{
    PageElParams p;
    page->layout->h.type = SCALE;
    page->layout->w.type = SCALE;
    page->layout->h.value = 1.0;
    page->layout->w.value = 1.0;
    Layout *container = layout_add_child(page->layout);

    container->y.value = 50;
    container->x.type = SCALE;
    container->x.value = 0.06;
    container->w.type = SCALE;
    container->w.value = 0.523333;
    container->h.type = PAD;
    int label_index = 0;
    char layout_name_buf[64] = {0};
    
    for (int i=0; i<num_els; i++) {
	AutoPageEl auto_el = els[i];
	switch (auto_el.ep->val_type) {
	case JDAW_FLOAT:
	case JDAW_DOUBLE:
	case JDAW_INT:
	case JDAW_UINT8:
	case JDAW_UINT16:
	case JDAW_UINT32:
	case JDAW_INT8:
	case JDAW_INT16:
	case JDAW_INT32: {
	    Layout *slider_and_label = layout_add_child(container);
	    slider_and_label->w.type = SCALE;
	    slider_and_label->w.value = 1.0;
	    slider_and_label->h.value = 60;
	    slider_and_label->y.type = STACK;
	    slider_and_label->y.value = 4.0;
	    Layout *label = layout_add_child(slider_and_label);
	    label->w.type = SCALE;
	    label->w.value = 1.0;
	    label->h.type = SCALE;
	    label->h.value = 0.5;
	    snprintf(layout_name_buf, sizeof(layout_name_buf), "lbl%d", label_index);
	    label_index++;
	    layout_set_name(label, layout_name_buf);
	    p.textbox_p.font = main_win->mono_bold_font;
	    p.textbox_p.set_str = auto_el.label_if_different ? (char *)auto_el.label_if_different : (char *)auto_el.ep->display_name;
	    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
	    p.textbox_p.win = main_win;
	    page_add_el(page, EL_TEXTBOX, p, "", layout_name_buf);
	    
	    Layout *slider = layout_add_child(slider_and_label);
	    snprintf(layout_name_buf, sizeof(layout_name_buf), "%ssldr", auto_el.ep->local_id);
	    layout_set_name(slider, layout_name_buf);
	    slider->y.type = STACK;
	    slider->w.type = SCALE;
	    slider->w.value = 1.0;
	    slider->h.type = SCALE;
	    slider->h.value = 0.5;

	    page_el_params_slider_from_ep(&p, auto_el.ep);
	    p.slider_p.orientation = SLIDER_HORIZONTAL;
	    p.slider_p.style = auto_el.slider_style;
	    page_add_el(page, EL_SLIDER, p, auto_el.ep->local_id, layout_name_buf);
	}
	    break;
	case JDAW_BOOL: {
	    Layout *toggle_and_label = layout_add_child(container);
	    toggle_and_label->w.type = SCALE;
	    toggle_and_label->w.value = 1.0;
	    toggle_and_label->h.value = 40;
	    toggle_and_label->y.type = STACK;
	    toggle_and_label->y.value = 0.0;

	    Layout *toggle = layout_add_child(toggle_and_label);
	    snprintf(layout_name_buf, sizeof(layout_name_buf), "%stgl", auto_el.ep->local_id);
	    layout_set_name(toggle, layout_name_buf);
	    toggle->w.value = 20;
	    toggle->h.value = 20;
	    p.toggle_p.ep = auto_el.ep;
	    page_add_el(page, EL_TOGGLE, p, auto_el.ep->local_id, layout_name_buf);

	    Layout *label = layout_add_child(toggle_and_label);
	    label->x.type = STACK;
	    label->x.value = 24;
	    /* label->y.value = 0; */
	    label->w.type = SCALE;
	    label->w.value = 1.0;
	    label->h.type = SCALE;
	    label->h.value = 0.5;
	    snprintf(layout_name_buf, sizeof(layout_name_buf), "lbl%d", label_index);
	    label_index++;
	    layout_set_name(label, layout_name_buf);
	    p.textbox_p.font = main_win->mono_bold_font;
	    p.textbox_p.set_str = auto_el.label_if_different ? (char *)auto_el.label_if_different : (char *)auto_el.ep->display_name;
	    p.textbox_p.text_size = LABEL_STD_FONT_SIZE;
	    p.textbox_p.win = main_win;
	    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "", layout_name_buf);
	    textbox_set_align(el->component, CENTER_LEFT);
	    
	}
	    break;
	default:
	    break;
	}
	
    }
}
