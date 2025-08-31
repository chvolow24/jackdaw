#include "animation.h"
#include "color.h"
#include "label.h"
#include "layout.h"
#include "session.h"
#include "value.h"

extern Window *main_win;
extern struct colors colors;

static void std_str_fn(char *dst, size_t dstsize, Value v, ValType t)
{
    jdaw_val_to_str(dst, dstsize, v, t, 2);
}

/* Use zero for default max len */
Label *label_create(
    int max_len,
    Layout *parent_lt,
    LabelStrFn set_str_fn,
    void *target_obj,
    ValType t,
    Window *win)
{
    Label *l = calloc(1, sizeof(Label));
    if (max_len == 0) {
	l->max_len = LABEL_DEFAULT_MAXLEN;
    } else {
	l->max_len = max_len;
    }
    l->str = calloc(l->max_len, sizeof(char));
    Layout *lt = layout_add_child(parent_lt);
    l->tb = textbox_create_from_str(l->str, lt, win->mono_font, 12, win);
    if (set_str_fn) {
	l->set_str_fn = set_str_fn;
    } else {
	l->set_str_fn = std_str_fn;
    }
    l->target_obj = target_obj;
    l->countdown_max = LABEL_COUNTDOWN_MAX;
    l->val_type = t;

    textbox_set_border(l->tb, &colors.black, 2);
    textbox_set_trunc(l->tb, false);
    textbox_size_to_fit(l->tb, LABEL_H_PAD, LABEL_V_PAD);

    return l;
}


static void label_draw_deferred(void *label_v)
{
    Label *label = (Label *)label_v;
    textbox_draw(label->tb);
}



void label_draw(Label *label)
{
    
    if (label->countdown_timer > 0) {
	window_defer_draw(main_win, label_draw_deferred, label);
	/* label->countdown_timer--; */
    }
}

void label_move(Label *label, int x, int y)
{
    label->tb->layout->rect.x = x;
    label->tb->layout->rect.y = y;
    layout_set_values_from_rect(label->tb->layout);
}


#include "project.h"
extern Project *proj;
static void animation_frame_op(void *arg1, void *arg2)
{
    Label *l = (Label *)arg1;
    if (l->countdown_timer > 0) 
	l->countdown_timer--;
}
static void animation_end_op(void *arg1, void *arg2)
{
    Session *session = session_get();
    Label *l = (Label *)arg1;
    if (l->countdown_timer <= 0) {
	l->animation_running = false;
	Timeline *tl = ACTIVE_TL;
	tl->needs_redraw = true;
    } else {
	l->animation = session_queue_animation(
	    animation_frame_op, animation_end_op,
	    (void *)l, NULL,
	    l->countdown_timer);

    }
}

void label_reset(Label *label, Value v)
{
    label->set_str_fn(label->str, label->max_len, v, label->val_type);
    textbox_size_to_fit(label->tb, LABEL_H_PAD, LABEL_V_PAD);

    /* Do not allow label to extend offscreen (right) */
    if (label->tb->layout->rect.x + label->tb->layout->rect.w > main_win->w_pix) {
	label->tb->layout->rect.x = main_win->w_pix - label->tb->layout->rect.w;
	layout_set_values_from_rect(label->tb->layout);
    }
    /* Do not allow label to intersect parent object layout (vertically) */
    if (label->parent_obj_lt && SDL_HasIntersection(&label->tb->layout->rect, &label->parent_obj_lt->rect)) {
	label->tb->layout->rect.y = label->parent_obj_lt->rect.y + label->parent_obj_lt->rect.h + 5 * main_win->dpi_scale_factor;
	layout_set_values_from_rect(label->tb->layout);
    }
    label->countdown_timer = label->countdown_max;
    if (!label->animation_running) {
	label->animation = session_queue_animation(
	    animation_frame_op, animation_end_op,
	    (void *)label, NULL,
	    label->countdown_max);
	label->animation_running = true;

    }
    textbox_reset_full(label->tb);
}

void label_destroy(Label *label)
{
    if (label->animation_running) {
	Animation *a = label->animation;
	session_dequeue_animation(a);
    }
    free(label->str);
    textbox_destroy(label->tb);
    free(label);
}
    



/* LABELMAKING UTILITIES */

static float amp_to_db(float amp)
{
    return (20.0f * log10(amp));
}

void label_amp_to_dbstr(char *dst, size_t dstsize, Value val, ValType t)
{
    float amp = t == JDAW_FLOAT ? val.float_v : val.double_v;
    float db = amp_to_db(amp);
    
    snprintf(dst, dstsize - 4, "%.*f", 2, db);
    /* jdaw_val_set_str(dst, dstsize - 4, val, t, 2); */
    strcat(dst, " dB");
}

void label_pan(char *dst, size_t dstsize, Value val, ValType t)
{
    float pan = t == JDAW_FLOAT ? val.float_v : val.double_v;
    if (pan < 0.5) {	
	snprintf(dst, dstsize, "%.1f%% L", (0.5 - pan) * 200);
    } else if (pan > 0.5) {
	snprintf(dst, dstsize, "%.1f%% R", (pan - 0.5) * 200);
    } else {
	snprintf(dst, dstsize, "C");
    }
}

    

/* void OLDlabel_amp_to_dbstr(char *dst, size_t dstsize, float amp) */
/* { */
/*     int max_float_chars = dstsize - 2; */
/*     if (max_float_chars < 1) { */
/* 	fprintf(stderr, "Error: no room for dbstr\n"); */
/* 	dst[0] = '\0'; */
/* 	return; */
/*     } */
/*     snprintf(dst, max_float_chars, "%.2f", amp_to_db(amp)); */
/*     strcat(dst, " dB"); */
/* } */

/* void label_pan(char *dst, size_t dstsize, float pan) */
/* { */
/*     if (pan < 0.5) {	 */
/* 	snprintf(dst, dstsize, "%.1f%% L", (0.5 - pan) * 200); */
/*     } else if (pan > 0.5) { */
/* 	snprintf(dst, dstsize, "%.1f%% R", (pan - 0.5) * 200); */
/*     } else { */
/* 	snprintf(dst, dstsize, "C"); */
/*     } */
/* } */


double dsp_scale_freq_to_hz(double raw);

void label_freq_raw_to_hz(char *dst, size_t dstsize, Value v, ValType t)
{
    double raw = t == JDAW_FLOAT ? v.float_v : v.double_v;
    double hz = dsp_scale_freq_to_hz(raw);
    snprintf(dst, dstsize, "%.0f Hz", hz);
    dst[dstsize - 1] = '\0';
}

void label_msec(char *dst, size_t dstsize, Value v, ValType t)
{
    jdaw_val_to_str(dst, dstsize, v, t, 2);
    strcat(dst, " ms");
}

void label_int_plus_one(char *dst, size_t dstsize, Value v, ValType t)
{
    int plusone = v.int_v + 1;
    snprintf(dst, dstsize, "%d", plusone);
    
}

/* void label_freq_raw_to_hz(char *dst, size_t dstsize, double raw) */
/* { */
/*     double hz = dsp_scale_freq_to_hz(raw); */
/*     snprintf(dst, dstsize, "%.0f Hz", hz); */
/*     /\* snprintf(dst, dstsize, "hi"); *\/ */
/*     dst[dstsize - 1] = '\0'; */
/* } */


/* void label_time_samples_to_msec(char *dst, size_t dstsize, Value v, ValType t) */
/* { */
    
/*     int msec = (double)samples * 1000 / sample_rate; */
/*     snprintf(dst, dstsize, "%d ms", msec); */
/* } */


/* void label_time_samples_to_msec(char *dst, size_t dstsize, int32_t samples, int32_t sample_rate) */
/* { */
/*     int msec = (double)samples * 1000 / sample_rate; */
/*     snprintf(dst, dstsize, "%d ms", msec); */
/* } */

