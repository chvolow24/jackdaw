#include <stdbool.h>
#include <stdlib.h>
#include "schroeder.h"
#include "allpass.h"
#include "dsp_utils.h"
#include "status.h"

static bool do_allpass = true;
static bool do_lop_delay = true;

void schroeder_toggle_do_allpass()
{
    do_allpass = !do_allpass;
    status_set_alertstr(do_allpass ? "allpass ON" : "allpass OFF");

}

void schroeder_toggle_do_lop_delay()
{
    do_lop_delay = !do_lop_delay;
    status_set_alertstr(do_lop_delay ? "lop delay ON" : "lop delay OFF");
}

static float cross_corr_coefficient = 0.0;

void schroeder_sample(Schroeder *sch, float in_L, float in_R, float *out_L, float *out_R)
{
    float allpassed_L = do_allpass ? allpass_group_sample(&sch->series_aps[0], in_L) : in_L;
    float allpassed_R = do_allpass ? allpass_group_sample(&sch->series_aps[1], in_R) : in_R;
    float intermed_L = 0.0f;
    float intermed_R = 0.0f;
    
    if (sch->num_parallel_lop_delays == 0 || !do_lop_delay) {
	intermed_L = allpassed_L;
	intermed_R = allpassed_R;
    } else {
	float L_lop;
	float R_lop;
	for (int i=0; i<sch->num_parallel_lop_delays; i++) {
	    /* float pan = (i < 4 && i % 2 == 0) || (i >= 4 && i % 2 != 0) ? 0.5 + sch->stereo_spread / 2 : 0.5 - sch->stereo_spread / 2; */
	    /* float pan_scale_L = pan_scale(pan, 0); */
	    /* float pan_scale_R = pan_scale(pan, 1); */
	    
	    /* fprintf(stderr, "Chan %d pan scale L: %f R: %f\n", i, pan_scale_L, pan_scale_R); */
	    L_lop = lop_delay_sample(&sch->parallel_lop_delays[0][i], allpassed_L) / sch->num_parallel_lop_delays;
	    R_lop = lop_delay_sample(&sch->parallel_lop_delays[1][i], allpassed_R) / sch->num_parallel_lop_delays;
	    /* intermed_L += pan_scale_L * (L_lop + R_lop) / 2; */
	    /* intermed_R += pan_scale_R * (L_lop + R_lop) / 2; */
	    if (i % 2 == 0) {
		float swap = L_lop;
		L_lop = R_lop;
		R_lop = swap;
	    }
	    float ster = sch->stereo_spread / 2; // range 0-0.5
	    intermed_L += (0.5f + ster) * L_lop + (0.5f - ster) * R_lop;
	    intermed_R += (0.5f - ster) * L_lop + (0.5f + ster) * R_lop;
	    /* if (i > 0 && cross_corr_coefficient > 0.0f) { */
	    /* 	sch->parallel_lop_delays[0][i].mem[sch */
	    /* } */
	    /* intermed_L += pan_scale_L * /\* pan_scale((float)i / sch->num_parallel_lop_delays, 0) * *\/ lop_delay_sample(&sch->parallel_lop_delays[0][i], allpassed_L + allpassed_R) / sch->num_parallel_lop_delays; */
	    /* intermed_R += pan_scale_R *  /\* pan_scale((float)i / sch->num_parallel_lop_delays, 1) * *\/ lop_delay_sample(&sch->parallel_lop_delays[1][i], allpassed_R) / sch->num_parallel_lop_delays; */
	}
    }
    *out_L = intermed_L;
    *out_R = intermed_R;
}

float schroeder_buf_apply(void *sch_v, float *restrict in_L, float *restrict in_R, int len, float input_amp)
{
    Schroeder *sch = sch_v;
    for (int32_t i=0; i<len; i++) {
	float old_L = in_L[i];
	float old_R = in_R[i];
	schroeder_sample(sch, in_L[i], in_R[i], in_L + i, in_R + i);
	in_L[i] = old_L * sch->dry + (1 - sch->dry) * in_L[i];
	in_R[i] = old_R * sch->dry + (1 - sch->dry) * in_R[i];
	
    }
    return input_amp;
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

void schroeder_init_freeverb(Schroeder *sch)
{

    sch->allpass_coeff = 0.5;
    sch->lop_coeff = 0.5;
    sch->lop_delay_coeff = 0.87;
    int32_t lens[4] = {
	225 * 96000 / 44100,
	556 * 96000 / 44100,
	441 * 96000 / 44100,
	341 * 96000 / 44100
	/* 3433, 4129, 6047, 7727 */
    };
    int32_t lens_r[4];
    for (int i=0; i<4; i++) {
	lens_r[i] = lens[i];
	    /* lens_r[i] += stereo_offset; */
	/* if (i % 2 == 0) { */
	/* } else { */
	/*     lens_r[i] -= stereo_offset; */
	/* } */
    }
    allpass_group_init(&sch->series_aps[0], 4, lens, sch->allpass_coeff);
    allpass_group_init(&sch->series_aps[1], 4, lens_r, sch->allpass_coeff);
    sch->num_parallel_lop_delays = 8;
    int32_t lop_lens[8] = {
	1617 * 96000 / 44100,
	1557 * 96000 / 44100,
	1491 * 96000 / 44100,
	1422 * 96000 / 44100,
	1356 * 96000 / 44100,
	1277 * 96000 / 44100,
	1188 * 96000 / 44100,
	1116 * 96000 / 44100
    };
    int32_t lop_lens_r[8] = {
	1667 * 96000 / 44100,
	1559 * 96000 / 44100,
	1537 * 96000 / 44100,
	1409 * 96000 / 44100,
	1361 * 96000 / 44100,
	1289 * 96000 / 44100,
	1223 * 96000 / 44100,
	1103 * 96000 / 44100	
    };
    for (int i=0; i<8; i++) {
	/* lop_lens_r[i] = lop_lens[i]; */
	/* if (i % 2 == 0) */
	    /* lop_lens_r[i] += stereo_offset; */
	/* else lop_lens_r[i] -= stereo_offset; */
	/* float lop_adj = (float)i / 8 * lop_adj_range * 2 - lop_adj_range; */
	lop_delay_init(sch->parallel_lop_delays[0] + i, lop_lens[i], sch->lop_delay_coeff, sch->lop_coeff);
	lop_delay_init(sch->parallel_lop_delays[1] + i, lop_lens_r[i], sch->lop_delay_coeff, sch->lop_coeff);
    }
    /* sch->num_parallel_lop_delays = 8; */
}

void schroeder_set_lop_coeff(Schroeder *sch, float new)
{
    sch->lop_coeff = new;
    for (int i=0; i<sch->num_parallel_lop_delays; i++) {
	sch->parallel_lop_delays[0][i].lop_coeff = new;
	sch->parallel_lop_delays[1][i].lop_coeff = new;
    }
}
void schroeder_set_lop_delay_coeff(Schroeder *sch, float new)
{
    sch->lop_delay_coeff = new;
    for (int i=0; i<sch->num_parallel_lop_delays; i++) {
	sch->parallel_lop_delays[0][i].delay_coeff = new;
	sch->parallel_lop_delays[1][i].delay_coeff = new;
    }
}
void schroeder_set_allpass_coeff(Schroeder *sch, float new)
{
    sch->allpass_coeff = new;
    allpass_group_set_coeff(&sch->series_aps[0], new);
    allpass_group_set_coeff(&sch->series_aps[1], new);
}

void schroeder_incr_lop_coeff(Schroeder *sch)
{
    float new = sch->lop_coeff + 0.01;
    if (new > 1.0) new = 1.0;
    schroeder_set_lop_coeff(sch, new);
}

void schroeder_decr_lop_coeff(Schroeder *sch)
{
    float new = sch->lop_coeff - 0.01;
    if (new < 0.0) new = 0.0;
    schroeder_set_lop_coeff(sch, new);
}

void schroeder_incr_lop_delay_coeff(Schroeder *sch)
{
    float new = sch->lop_delay_coeff + 0.01;
    if (new > 0.99) new = 0.99;
    schroeder_set_lop_delay_coeff(sch, new);
}

void schroeder_decr_lop_delay_coeff(Schroeder *sch)
{
    float new = sch->lop_delay_coeff - 0.01;
    if (new < 0.0) new = 0.0;
    schroeder_set_lop_delay_coeff(sch, new);
}

void schroeder_incr_allpass_coeff(Schroeder *sch)
{
    float new = sch->allpass_coeff + 0.1;
    if (new > 0.9) new = 0.9;
    schroeder_set_allpass_coeff(sch, new);
}

void schroeder_decr_allpass_coeff(Schroeder *sch)
{
    float new = sch->allpass_coeff - 0.1;
    if (new < -0.9) new = -0.9;
    schroeder_set_allpass_coeff(sch, new);
}

void schroeder_incr_dry(Schroeder *sch)
{
    float new = sch->dry + 0.1;
    if (new > 1.0) {
	new = 1.0;
    }
    sch->dry = new;
}

void schroeder_decr_dry(Schroeder *sch)
{
    float new = sch->dry - 0.1;
    if (new < 0.0) {
	new = 0.0;
    }
    sch->dry = new;
}

void schroeder_incr_stereo_spread(Schroeder *sch)
{
    float new = sch->stereo_spread + 0.1;
    if (new > 1.0) new = 1.0;
    sch->stereo_spread = new;
}

void schroeder_decr_stereo_spread(Schroeder *sch)
{
    float new = sch->stereo_spread - 0.1;
    if (new < 0.0) new = 0.0;
    sch->stereo_spread = new;
}




