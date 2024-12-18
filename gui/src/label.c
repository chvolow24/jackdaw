#include "label.h"
#include "layout.h"

extern SDL_Color color_global_black;

static void std_str_fn(char *dst, size_t dstsize, void *target, ValType t)
{
    jdaw_valptr_set_str(dst, dstsize, target, t, 2);
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

    /* textbox_set_pad(l->tb, LABEL_H_PAD, LABEL_V_PAD); */
    textbox_set_border(l->tb, &color_global_black, 2);
    textbox_set_trunc(l->tb, false);
    textbox_size_to_fit(l->tb, LABEL_H_PAD, LABEL_V_PAD);

    return l;
}

void label_draw(Label *label)
{
    if (label->countdown_timer > 0) {
	textbox_draw(label->tb);
	label->countdown_timer--;
    }
}

void label_move(Label *label, int x, int y)
{
    label->tb->layout->rect.x = x;
    label->tb->layout->rect.y = y;
    layout_set_values_from_rect(label->tb->layout);
}
void label_reset(Label *label)
{
    label->set_str_fn(label->str, label->max_len, label->target_obj, label->val_type);
    textbox_size_to_fit(label->tb, LABEL_H_PAD, LABEL_V_PAD);
    label->countdown_timer = label->countdown_max;
    textbox_reset_full(label->tb);
}


void label_destroy(Label *label)
{
    free(label->str);
    textbox_destroy(label->tb);
    free(label);
}



/* LABELMAKING UTILITIES */

static float amp_to_db(float amp)
{
    return (20.0f * log10(amp));
}

void label_amp_to_dbstr(char *dst, size_t dstsize, float amp)
{
    int max_float_chars = dstsize - 2;
    if (max_float_chars < 1) {
	fprintf(stderr, "Error: no room for dbstr\n");
	dst[0] = '\0';
	return;
    }
    snprintf(dst, max_float_chars, "%.2f", amp_to_db(amp));
    strcat(dst, " dB");
}

void label_pan(char *dst, size_t dstsize, float pan)
{
    if (pan < 0.5) {	
	snprintf(dst, dstsize, "%.1f%% L", (0.5 - pan) * 200);
    } else if (pan > 0.5) {
	snprintf(dst, dstsize, "%.1f%% R", (pan - 0.5) * 200);
    } else {
	snprintf(dst, dstsize, "C");
    }
}


double dsp_scale_freq_to_hz(double raw);
void label_freq_raw_to_hz(char *dst, size_t dstsize, double raw)
{
    double hz = dsp_scale_freq_to_hz(raw);
    snprintf(dst, dstsize, "%.0f Hz", hz);
    /* snprintf(dst, dstsize, "hi"); */
    dst[dstsize - 1] = '\0';
}


void label_time_samples_to_msec(char *dst, size_t dstsize, int32_t samples, int32_t sample_rate)
{
    int msec = (double)samples * 1000 / sample_rate;
    snprintf(dst, dstsize, "%d ms", msec);
}
