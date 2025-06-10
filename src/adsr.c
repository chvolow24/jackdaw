#include <stdlib.h>
#include "adsr.h"



void adsr_set_params(ADSRParams *p, int32_t a, int32_t d, float s, int32_t r)
{
    p->a = a;
    p->d = d;
    p->s = s;
    p->r = r;

    if (p->a_ramp) free(p->a_ramp);
    if (p->d_ramp) free(p->d_ramp);
    /* if (p->r_ramp) free(p->r_ramp); */
    p->a_ramp = malloc(sizeof(float) * a);
    p->d_ramp = malloc(sizeof(float) * d);
    for (int32_t i=0; i<a; i++) {
	p->a_ramp[i] = (double)i / a;
    }
    for (int32_t i=0; i<d; i++) {
	p->d_ramp[i] = 1.0 - (float)i * (1.0 - s)/d;
    }
}


static void adsr_start_release(ADSRState *s)
{
    s->current_stage = ADSR_R;
    s->env_remaining = 0;
}
static float adsr_sample(ADSRState *s)
{
    float env = 1.0;
    switch(s->current_stage) {
    case ADSR_A:
	env = (s->env_remaining - s->params->a);
	break;
    case ADSR_D:
	env = (s->env_remaining - s->params->a);
	break;
    case ADSR_S:
	env = s->params->s;
	break;
    case ADSR_R:
	env = (float)s->env_remaining * s->release_start_env / (float)s->params->d;
	break;
    default:
	break;       
    }
    if (s->current_stage != ADSR_R)
	s->release_start_env = env;

    if(s->current_stage != ADSR_S)
	s->env_remaining--;
    
    /* RESET STAGES based on env_remaining */
}
