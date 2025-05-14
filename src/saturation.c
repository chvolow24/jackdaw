/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    saturation.c

    * tanh waveshaping saturator
 *****************************************************************************************************************/

#include "endpoint_callbacks.h"
#include "saturation.h"


static void saturation_gain_cb(Endpoint *ep)
{
    Saturation *s = (Saturation *)ep->xarg1;
    saturation_set_gain(s, s->gain);
}

static void saturation_type_cb(Endpoint *ep)
{
    Saturation *s = (Saturation *)ep->xarg1;
    saturation_set_type(s, s->type);
}

static void saturation_do_gain_comp_cb(Endpoint *ep)
{
    Saturation *s = (Saturation *)ep->xarg1;
    saturation_set_type(s, s->type);
}

void saturation_init(Saturation *s)
{
    s->do_gain_comp = true;
    saturation_set_gain(s, 1.0);
    saturation_set_type(s, SAT_TANH);

    endpoint_init(
	&s->gain_ep,
	&s->gain,
	JDAW_DOUBLE,
	"saturation_gain",
	"Saturation gain",
	JDAW_THREAD_DSP,
	track_settings_page_el_gui_cb, NULL,
	saturation_gain_cb,
	(void *)s, NULL, &s->effect->page, "track_settings_saturation_gain_slider");
    endpoint_set_allowed_range(&s->gain_ep, (Value){.double_v = 1.0}, (Value){.double_v = SATURATION_MAX_GAIN});
    endpoint_set_default_value(&s->gain_ep, (Value){.double_v = 1.0});
    endpoint_set_label_fn(&s->gain_ep, label_amp_to_dbstr);
    api_endpoint_register(&s->gain_ep, &s->effect->api_node);
    
    endpoint_init(
	&s->gain_comp_ep,
	&s->do_gain_comp,
	JDAW_BOOL,
	"saturation_gain_comp",
	"Saturation gain comp",
	JDAW_THREAD_DSP,
	NULL, NULL, saturation_do_gain_comp_cb,
	(void *)s, NULL, NULL, NULL);
    endpoint_init(
	&s->type_ep,
	&s->type,
	JDAW_BOOL,
	"saturation_type",
	"Saturation type",
	JDAW_THREAD_DSP,
	track_settings_page_el_gui_cb, NULL, saturation_type_cb,
	(void *)s, NULL, &s->effect->page, "track_settings_saturation_type");
}

static double saturation_sample_tanh(Saturation *s, double in)
{
    return tanh(in * s->gain) * s->gain_comp_val;
}

static double saturation_sample_tanh_no_gain_comp(Saturation *s, double in)
{
    return tanh(in * s->gain);
}

static double saturation_sample_exponential_no_gain_comp(Saturation *s, double in)
{
    /* return tanh(2 * in * s->gain - pow(in * s->gain, 3.0) / 3.0); */
    int sign = in < 0 ? -1 : 1;
    return sign * (1.0-exp(-1 * fabs(in * s->gain)));
    /* return 2.0 / (1 + exp(in * -2 * s->gain)) - 1; */
}
static double saturation_sample_exponential(Saturation *s, double in)
{
    return saturation_sample_exponential_no_gain_comp(s, in) * s->gain_comp_val;
    /* return sign * (1.0-exp(-1 * fabs(in * s->gain))); */
    /* return s->logistic_gain_comp_val * s->gain_comp_val * (2.0 / (1 + exp(in * -2 * s->gain)) - 1); */
}



void saturation_set_gain(Saturation *s, double gain)
{
    s->gain = gain;
    s->gain_comp_val = 1.0 / (log(gain) + 1);
}

void saturation_set_type(Saturation *s, SaturationType t)
{
    switch(t) {
    case SAT_TANH:
	s->sample_fn = s->do_gain_comp ? saturation_sample_tanh : saturation_sample_tanh_no_gain_comp;
	break;
    case SAT_EXPONENTIAL:
	s->sample_fn = s->do_gain_comp ? saturation_sample_exponential : saturation_sample_exponential_no_gain_comp;
	break;
    default:
	break;
    }
}

static double saturation_sample(Saturation *s, double in)
{
    /* if (!s->active) return in; */
    return s->sample_fn(s, in);
    /* return tanh(in * s->amp); */
}

float saturation_buf_apply(void *saturation_v, float *buf, int len, int channel_unused, float input_amp)
{
    Saturation *s = saturation_v;
    /* if (!s->active) return input_amp; */
    float output_amp = 0.0f;
    for (int i=0; i<len; i++) {
	buf[i] = saturation_sample(s, buf[i]);
	output_amp += fabs(buf[i]);
    }
    return output_amp;
}


/* void saturation_deinit(Saturation *s) */
/* { */
/*     /\* TODO: de-register endpoints ? *\/ */
/* } */



/* TODO:
   - calculate gain comp whenever amp is set
   - fn pointer for sample calc, modified when type is changed
   - gain comp on by default. Maybe rename "disable gain comp"

 */
