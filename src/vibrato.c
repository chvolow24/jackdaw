#include "consts.h"
#include "endpoint.h"
#include "endpoint_callbacks.h"
#include "mod_delay.h"
#include "session.h"
#include "vibrato.h"

#define MAX_VIB_DELAY_LEN (session_get_sample_rate() / 2)
#define VIB_DEFAULT_DEPTH 0.28
#define VIB_DEFAULT_FREQ_HZ 4.0
#define VIB_DEFAULT_FREQ_UNSCALED (vib_unscale_freq(VIB_DEFAULT_FREQ_HZ))
#define VIB_MAX_FREQ_HZ 200.0
#define VIB_HALF_FREQ_HZ 10.0

static double vib_scale_freq(double ctrl)
{
    const double n = log(VIB_HALF_FREQ_HZ / VIB_MAX_FREQ_HZ) / log(0.5);
    return VIB_MAX_FREQ_HZ * pow(ctrl, n);
}
static double vib_unscale_freq(double scaled)
{
    const double n = log(VIB_HALF_FREQ_HZ / VIB_MAX_FREQ_HZ) / log(0.5);
    return pow(scaled / VIB_MAX_FREQ_HZ, 1/n);
}

LabelFnDef(vib_freq_labelfn)
{
    double ctrl = val.double_v;
    double out = vib_scale_freq(ctrl);
    snprintf(dst, dstsize, "%.1f Hz", out);
}

/*
  - Ring buffer already moves at playspeed 1.0
  - slope of oscillator defines playspeed adjustment
  - slope of a * sin(fx) = f * a * cos(fx)
  - in other words, slope is a factor of f and a!
  - at f = 0, it should be a, not f * a;
  - at f = 1, it should be a (GOOD)
  - so desired depth D => f * a
  - multiply a * 1/f?
  
*/

static double depth_from_ctrl_and_freq(double depth_ctrl, double freq_hz)
{
    return pow(depth_ctrl, 2.0) / (PI * freq_hz);
}

static void freq_dsp_cb(Endpoint *ep)
{
    double ctrl = ep->current_write_val.double_v;
    Vibrato *vib = ep->xarg1;
    vib->freq_hz = vib_scale_freq(ctrl);
    mod_delay_set_freq(&vib->mdL, vib->freq_hz);
    mod_delay_set_freq(&vib->mdR, vib->freq_hz);
    vib->depth = depth_from_ctrl_and_freq(vib->depth_ctrl, vib->freq_hz);
    mod_delay_set_amp(&vib->mdL, vib->depth);
    mod_delay_set_amp(&vib->mdR, vib->depth);

}

static void depth_dsp_cb(Endpoint *ep)
{
    double depth_ctrl = ep->current_write_val.double_v;
    Vibrato *vib = ep->xarg1;
    /* vib->depth = pow(depth_ctrl, 4.0); */
    vib->depth = depth_from_ctrl_and_freq(depth_ctrl, vib->freq_hz);
    mod_delay_set_amp(&vib->mdL, vib->depth);
    mod_delay_set_amp(&vib->mdR, vib->depth);
}


void vibrato_init(Vibrato *vib)
{
    mod_delay_init(
	&vib->mdL,
	MAX_VIB_DELAY_LEN,
	VIB_DEFAULT_DEPTH,
	VIB_DEFAULT_FREQ_HZ,
	1,
	OSC_SINE);

    mod_delay_init(
	&vib->mdR,
	MAX_VIB_DELAY_LEN,
	VIB_DEFAULT_DEPTH,
	VIB_DEFAULT_FREQ_HZ,
	1,
	OSC_SINE);

    endpoint_init(
	&vib->freq_hz_ep,
	&vib->freq_ctrl,
	JDAW_DOUBLE,
	"freq_hz",
	"Freq hz",
	JDAW_THREAD_DSP,
	component_gui_cb, NULL, freq_dsp_cb,
	vib, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&vib->freq_hz_ep,
	(Value){.double_v = 0.1},
	(Value){.double_v = 1.0});
    endpoint_set_default_value(&vib->freq_hz_ep, (Value){.double_v = VIB_DEFAULT_FREQ_UNSCALED});
    endpoint_write_default(&vib->freq_hz_ep);
    endpoint_set_label_fn(&vib->freq_hz_ep, vib_freq_labelfn);
    api_endpoint_register(&vib->freq_hz_ep, &vib->effect->api_node);

    endpoint_init(
	&vib->depth_ep,
	&vib->depth_ctrl,
	JDAW_DOUBLE,
	"depth",
	"Depth",
	JDAW_THREAD_DSP,
	component_gui_cb, NULL, depth_dsp_cb,
	vib, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&vib->depth_ep,
	(Value){.double_v = 0.0},
	(Value){.double_v = 1.0});
    endpoint_set_default_value(&vib->depth_ep, (Value){.double_v = VIB_DEFAULT_DEPTH});
    endpoint_write_default(&vib->depth_ep);
    api_endpoint_register(&vib->depth_ep, &vib->effect->api_node);
    
	
}

float vibrato_buf_apply(void *vib_v, float *restrict L, float *restrict R, int len, float input_amp)
{
    Vibrato *vib = vib_v;
    if (L)
	mod_delay_buf(&vib->mdL, L, len);
    if (R) {
	mod_delay_buf(&vib->mdR, R, len);
	if (L) {
	    vib->mdR.phase = vib->mdL.phase;
	}
    }
    
    return input_amp;

}
void vibrato_clear(Vibrato *vib)
{
    mod_delay_clear(&vib->mdL);
    mod_delay_clear(&vib->mdR);
}

void vibrato_deinit(Vibrato *vib)
{
    mod_delay_deinit(&vib->mdL);
    mod_delay_deinit(&vib->mdR);
}

