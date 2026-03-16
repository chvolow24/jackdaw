#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "endpoint.h"
#include "schroeder.h"
#include "allpass.h"
#include "endpoint_callbacks.h"
#include "session.h"

float schroeder_buf_apply_mono(Schroeder *sch, float *restrict in, int len)
{
    float output_amp = 0.0f;
    for (int32_t i=0; i<len; i++) {
	float dry_L = in[i];
	float reverb_in_L;
	if (sch->predelay_len > 0) {
	    reverb_in_L = sch->predelay_buf[0][sch->predelay_index];	    
	    sch->predelay_buf[0][sch->predelay_index] = dry_L;
	    sch->predelay_index++;
	    if (sch->predelay_index >= sch->predelay_len) {
		sch->predelay_index -= sch->predelay_len;
	    }
	} else {
	    reverb_in_L = dry_L;
	}
	float allpassed_L = allpass_group_sample(&sch->series_aps[0], reverb_in_L);
	float intermed_L = 0.0f;    
	for (int i=0; i<SCHROEDER_NUM_PARALLEL_LOP_DELAYS; i++) {
	    intermed_L += lop_delay_sample(&sch->parallel_lop_delays[0][i], allpassed_L) / SCHROEDER_NUM_PARALLEL_LOP_DELAYS;
	}
	in[i] = dry_L * (1 - sch->wet) + sch->wet * intermed_L;
	output_amp += fabs(in[i]);
    }
    return output_amp;

}

float schroeder_buf_apply(void *sch_v, float *restrict in_L, float *restrict in_R, int len, float input_amp)
{
    Schroeder *sch = sch_v;
    float output_amp = 0.0f;
    if (!in_R) {
	return schroeder_buf_apply_mono(sch, in_L, len);
    } else if (!in_L) {
	return schroeder_buf_apply_mono(sch, in_R, len);
    }
    for (int32_t i=0; i<len; i++) {
	float dry_L = in_L[i];
	float dry_R = in_R[i];
	float reverb_in_L, reverb_in_R;
	if (sch->predelay_len > 0) {
	    reverb_in_L = sch->predelay_buf[0][sch->predelay_index];	    
	    reverb_in_R = sch->predelay_buf[1][sch->predelay_index];
	    sch->predelay_buf[0][sch->predelay_index] = dry_L;
	    sch->predelay_buf[1][sch->predelay_index] = dry_R;
	    sch->predelay_index++;
	    if (sch->predelay_index >= sch->predelay_len) {
		sch->predelay_index -= sch->predelay_len;
	    }
	} else {
	    reverb_in_L = dry_L;
	    reverb_in_R = dry_R;;
	}
	/* schroeder_sample(sch, in_L[i], in_R[i], in_L + i, in_R + i); */
	float allpassed_L = allpass_group_sample(&sch->series_aps[0], reverb_in_L);
	float allpassed_R = allpass_group_sample(&sch->series_aps[1], reverb_in_R);
	float intermed_L = 0.0f;
	float intermed_R = 0.0f;
    
	float L_lop;
	float R_lop;
	bool even = true;
	for (int i=0; i<SCHROEDER_NUM_PARALLEL_LOP_DELAYS; i++) {
	    L_lop = lop_delay_sample(&sch->parallel_lop_delays[0][i], allpassed_L) / SCHROEDER_NUM_PARALLEL_LOP_DELAYS;
	    R_lop = lop_delay_sample(&sch->parallel_lop_delays[1][i], allpassed_R) / SCHROEDER_NUM_PARALLEL_LOP_DELAYS;
	    if (even) {
		float swap = L_lop;
		L_lop = R_lop;
		R_lop = swap;
		even = false;
	    } else {
		even = true;
	    }
	    /* float ster = sch->stereo_spread / 2; // range 0-0.5 */
	    intermed_L += sch->panscale_left * L_lop + sch->panscale_right * R_lop;
	    intermed_R += sch->panscale_right * L_lop + sch->panscale_left * R_lop;
	}
	/* *out_L = intermed_L; */
	/* *out_R = intermed_R; */

	in_L[i] = dry_L * (1 - sch->wet) + sch->wet * intermed_L;
	in_R[i] = dry_R * (1 - sch->wet) + sch->wet * intermed_R;
	output_amp += fabs(in_L[i]);
	output_amp += fabs(in_R[i]);
    }
    return output_amp;
}


/* static int32_t gcd(int32_t a, int32_t b) */
/* { */
/*     if (a == b) return a; */
/*     if (a > b) return gcd(a - b, b); */
/*     else return gcd(b - a, a); */
/* } */

/* static int get_coprimes_in_range(int32_t max, int32_t min, int32_t **dst) */
/* { */
/*     int alloc_len = 4; */
/*     int32_t *coprimes = malloc(alloc_len * sizeof(int)); */
/*     coprimes[0] = max; */
/*     int num_coprimes = 1; */
/*     int32_t test = max - 1; */
/*     while (test >= min) { */
/* 	bool append = false; */
/* 	if (gcd(test, max) == 1) { */
/* 	    append = true; */
/* 	    for (int32_t i=0; i<num_coprimes; i++) { */
/* 		if (gcd(test, coprimes[i]) != 1) { */
/* 		    append = false; */
/* 		    break; */
/* 		} */
/* 	    } */
/* 	} */
/* 	if (append) { */
/* 	    if (num_coprimes == alloc_len) { */
/* 		alloc_len *= 2; */
/* 		coprimes = realloc(coprimes, alloc_len * sizeof(int)); */
/* 	    } */
/* 	    coprimes[num_coprimes] = test; */
/* 	    num_coprimes++; */
/* 	} */
/* 	test--; */
/*     } */
/*     *dst = coprimes; */
/*     return num_coprimes; */
/* } */

void schroeder_set_lop_coeff(Schroeder *sch, float new)
{
    sch->lop_coeff = new;
    for (int i=0; i<SCHROEDER_NUM_PARALLEL_LOP_DELAYS; i++) {
	sch->parallel_lop_delays[0][i].lop_coeff = new;
	sch->parallel_lop_delays[1][i].lop_coeff = new;
    }
}
void schroeder_set_lop_delay_coeff(Schroeder *sch, float new)
{
    sch->lop_delay_coeff = new;
    for (int i=0; i<SCHROEDER_NUM_PARALLEL_LOP_DELAYS; i++) {
	sch->parallel_lop_delays[0][i].delay_coeff = new;
	sch->parallel_lop_delays[1][i].delay_coeff = new;
    }
}

void decay_time_dsp_cb(Endpoint *ep)
{
    Schroeder *sch = ep->xarg1;
    float raw = ep->current_write_val.float_v;
    /* sch->lop_delay_coeff = 0.5 + sqrt(raw) / 2; */
    sch->lop_delay_coeff_raw = 0.6 + 2 * sqrt(raw) / 5;
    sch->lop_delay_coeff = expf(sch->delay_len_scalar * logf(sch->lop_delay_coeff_raw));
    schroeder_set_lop_delay_coeff(sch, sch->lop_delay_coeff);
}

void brightness_dsp_cb(Endpoint *ep)
{
    Schroeder *sch = ep->xarg1;
    float raw = ep->current_write_val.float_v;
    /* float lop_coeff = expf(sqrtf(sch->delay_len_scalar) * logf(raw)); */
    /* fprintf(stderr, "Lop coeff %f (br %f)\n ", lop_coeff, raw); */
    schroeder_set_lop_coeff(sch, raw);
    /* sch->lop_coeff =  */
}

void stereo_spread_dsp_cb(Endpoint *ep)
{
    Schroeder *sch = ep->xarg1;
    float raw = ep->current_write_val.float_v;
    sch->panscale_left = 0.5f + raw / 2;
    sch->panscale_right = 0.5f - raw / 2;
}

void delay_len_scalar_dsp_cb(Endpoint *ep)
{
    Schroeder *sch = ep->xarg1;
    sch->lop_delay_coeff = expf(sch->delay_len_scalar * logf(sch->lop_delay_coeff_raw));
    schroeder_set_lop_delay_coeff(sch, sch->lop_delay_coeff);
    /* float lop_coeff = exp(sqrtf(sch->delay_len_scalar) * logf(sch->brightness)); */
    /* schroeder_set_lop_coeff(sch, lop_coeff); */

    for (int i=0; i<SCHROEDER_NUM_PARALLEL_LOP_DELAYS; i++) {
	lop_delay_set_len(sch->parallel_lop_delays[0] + i, sch->delay_len_scalar);
	lop_delay_set_len(sch->parallel_lop_delays[1] + i, sch->delay_len_scalar);
    }
}

void predelay_dsp_cb(Endpoint *ep)
{
    Schroeder *sch = ep->xarg1;
    float raw = ep->current_write_val.float_v;
    sch->predelay_index = 0;
    sch->predelay_len = (double)raw / 1000 * session_get_sample_rate();
}

void schroeder_init_freeverb(Schroeder *sch)
{
    sch->allpass_coeff = 0.5;
    sch->lop_coeff = 0.5;
    sch->lop_delay_coeff = 0.87;
    int32_t sr = session_get_sample_rate();
    int32_t lens[4] = {
	225 * sr / 44100,
	556 * sr / 44100,
	441 * sr / 44100,
	341 * sr / 44100
    };
    allpass_group_init(&sch->series_aps[0], 4, lens, sch->allpass_coeff);
    allpass_group_init(&sch->series_aps[1], 4, lens, sch->allpass_coeff);
    int32_t lop_lens[8] = {
	1617 * sr / 44100,
	1557 * sr / 44100,
	1491 * sr / 44100,
	1422 * sr / 44100,
	1356 * sr / 44100,
	1277 * sr / 44100,
	1188 * sr / 44100,
	1116 * sr / 44100
    };
    int32_t lop_lens_r[8] = {
	1667 * sr / 44100,
	1559 * sr / 44100,
	1537 * sr / 44100,
	1409 * sr / 44100,
	1361 * sr / 44100,
	1289 * sr / 44100,
	1223 * sr / 44100,
	1103 * sr / 44100	
    };

    for (int i=0; i<8; i++) {
	lop_delay_init(sch->parallel_lop_delays[0] + i, lop_lens[i], sch->lop_delay_coeff, sch->lop_coeff);
	lop_delay_init(sch->parallel_lop_delays[1] + i, lop_lens_r[i], sch->lop_delay_coeff, sch->lop_coeff);
    }


    /* Initialize endpooints */

    endpoint_init(
	&sch->decay_time_ep,
	&sch->decay_time,
	JDAW_FLOAT,
	"decay_time",
	"Decay time",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, decay_time_dsp_cb,
	sch, NULL, &sch->effect->page, "decay_time_slider");
    endpoint_set_allowed_range(&sch->decay_time_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
    endpoint_set_default_value(&sch->decay_time_ep, (Value){.float_v = 0.75f});
    endpoint_write_default(&sch->decay_time_ep);
    api_endpoint_register(&sch->decay_time_ep, &sch->effect->api_node);

    endpoint_init(
	&sch->brightness_ep,
	&sch->brightness,
	JDAW_FLOAT,
	"brightness",
	"Brightness",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, brightness_dsp_cb,
	sch, NULL, &sch->effect->page, "brightness_slider");
    endpoint_set_allowed_range(&sch->brightness_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
    endpoint_set_default_value(&sch->brightness_ep, (Value){.float_v = 0.40f});
    endpoint_write_default(&sch->brightness_ep);
    api_endpoint_register(&sch->brightness_ep, &sch->effect->api_node);
    
    endpoint_init(
	&sch->stereo_spread_ep,
	&sch->stereo_spread,
	JDAW_FLOAT,
	"stereo_spread",
	"Stereo spread",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, stereo_spread_dsp_cb,
	sch, NULL, &sch->effect->page, "stereo_spread_slider");
    endpoint_set_allowed_range(&sch->stereo_spread_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
    endpoint_set_default_value(&sch->stereo_spread_ep, (Value){.float_v = 1.0f});
    endpoint_write_default(&sch->stereo_spread_ep);
    api_endpoint_register(&sch->stereo_spread_ep, &sch->effect->api_node);

    endpoint_init(
	&sch->delay_len_scalar_ep,
	&sch->delay_len_scalar,
	JDAW_FLOAT,
	"delay_len_scalar",
	"Delay length scalar",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, delay_len_scalar_dsp_cb,
	sch, NULL, &sch->effect->page, "delay_len_scalar_slider");
    endpoint_set_allowed_range(&sch->delay_len_scalar_ep, (Value){.float_v = 0.01f}, (Value){.float_v = 1.0f});
    endpoint_set_default_value(&sch->delay_len_scalar_ep, (Value){.float_v = 1.0f});
    endpoint_write_default(&sch->delay_len_scalar_ep);
    api_endpoint_register(&sch->delay_len_scalar_ep, &sch->effect->api_node);
	

    endpoint_init(
	&sch->predelay_ep,
	&sch->predelay_msec,
	JDAW_FLOAT,
	"predelay",
	"Pre-delay",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, predelay_dsp_cb,
	sch, NULL, &sch->effect->page, "predelay_slider");
    endpoint_set_allowed_range(&sch->predelay_ep, (Value){.float_v = 0.0f}, (Value){.float_v = (double)MAX_PREDELAY_SFRAMES / session_get_sample_rate() * 1000});
    endpoint_set_default_value(&sch->predelay_ep, (Value){.float_v = 0.0f});
    endpoint_write_default(&sch->predelay_ep);
    endpoint_set_label_fn(&sch->predelay_ep, label_msec);
    api_endpoint_register(&sch->predelay_ep, &sch->effect->api_node);

    endpoint_init(
	&sch->wet_ep,
	&sch->wet,
	JDAW_FLOAT,
	"wet",
	"Wet",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, NULL,
	NULL, NULL, &sch->effect->page, "wet_slider");
    endpoint_set_allowed_range(&sch->wet_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
    endpoint_set_default_value(&sch->wet_ep, (Value){.float_v = 0.25f});
    endpoint_write_default(&sch->wet_ep);
    endpoint_set_label_fn(&sch->wet_ep, label_dry_wet_mix);
    api_endpoint_register(&sch->wet_ep, &sch->effect->api_node);
}

/* void schroeder_set_allpass_coeff(Schroeder *sch, float new) */
/* { */
/*     sch->allpass_coeff = new; */
/*     allpass_group_set_coeff(&sch->series_aps[0], new); */
/*     allpass_group_set_coeff(&sch->series_aps[1], new); */
/* } */

/* void schroeder_incr_lop_coeff(Schroeder *sch) */
/* { */
/*     float new = sch->lop_coeff + 0.01; */
/*     if (new > 1.0) new = 1.0; */
/*     schroeder_set_lop_coeff(sch, new); */
/* } */

/* void schroeder_decr_lop_coeff(Schroeder *sch) */
/* { */
/*     float new = sch->lop_coeff - 0.01; */
/*     if (new < 0.0) new = 0.0; */
/*     schroeder_set_lop_coeff(sch, new); */
/* } */

/* void schroeder_incr_lop_delay_coeff(Schroeder *sch) */
/* { */
/*     float new = sch->lop_delay_coeff + 0.01; */
/*     if (new > 0.99) new = 0.99; */
/*     schroeder_set_lop_delay_coeff(sch, new); */
/* } */

/* void schroeder_decr_lop_delay_coeff(Schroeder *sch) */
/* { */
/*     float new = sch->lop_delay_coeff - 0.01; */
/*     if (new < 0.0) new = 0.0; */
/*     schroeder_set_lop_delay_coeff(sch, new); */
/* } */

/* void schroeder_incr_allpass_coeff(Schroeder *sch) */
/* { */
/*     float new = sch->allpass_coeff + 0.1; */
/*     if (new > 0.9) new = 0.9; */
/*     schroeder_set_allpass_coeff(sch, new); */
/* } */

/* void schroeder_decr_allpass_coeff(Schroeder *sch) */
/* { */
/*     float new = sch->allpass_coeff - 0.1; */
/*     if (new < -0.9) new = -0.9; */
/*     schroeder_set_allpass_coeff(sch, new); */
/* } */

/* void schroeder_incr_dry(Schroeder *sch) */
/* { */
/*     float new = sch->dry + 0.1; */
/*     if (new > 1.0) { */
/* 	new = 1.0; */
/*     } */
/*     sch->dry = new; */
/* } */

/* void schroeder_decr_dry(Schroeder *sch) */
/* { */
/*     float new = sch->dry - 0.1; */
/*     if (new < 0.0) { */
/* 	new = 0.0; */
/*     } */
/*     sch->dry = new; */
/* } */

/* void schroeder_incr_stereo_spread(Schroeder *sch) */
/* { */
/*     float new = sch->stereo_spread + 0.1; */
/*     if (new > 1.0) new = 1.0; */
/*     sch->stereo_spread = new; */
/* } */

/* void schroeder_decr_stereo_spread(Schroeder *sch) */
/* { */
/*     float new = sch->stereo_spread - 0.1; */
/*     if (new < 0.0) new = 0.0; */
/*     sch->stereo_spread = new; */
/* } */

void schroeder_clear(Schroeder *sch)
{
    allpass_group_clear(sch->series_aps);
    allpass_group_clear(sch->series_aps + 1);
    for (int32_t i=0; i<SCHROEDER_NUM_PARALLEL_LOP_DELAYS; i++) {
	lop_delay_clear(sch->parallel_lop_delays[0] + i);
	lop_delay_clear(sch->parallel_lop_delays[1] + i);
    }
    memset(sch->predelay_buf, 0, sizeof(float) * MAX_PREDELAY_SFRAMES * 2);
}



