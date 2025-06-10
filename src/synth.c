#include <portmidi.h>
#include <porttime.h>
#include "synth.h"
#include "session.h"

extern double MTOF[];

Synth *synth_create()
{
    Session *session = session_get();
    Synth *s = calloc(1, sizeof(Synth));
    s->num_oscs = 2;
    /* s->osc_types[0] = WS_SINE; */
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	s->voices[i].oscs[0].type = WS_SAW;
	s->voices[i].oscs[0].amp = 0.2;
	s->voices[i].oscs[1].type = WS_SAW;
	s->voices[i].oscs[1].amp = 0.2;

	s->voices[i].synth = s;
	s->voices[i].available = true;
	/* s->voices[i].amp_env_stage =  */
    }
    s->amp_env.a = 96 * 1;
    s->amp_env.d = 96 * 100;
    s->amp_env.s = 0.5;
    s->amp_env.r = 96 * 500;
    s->monitor = true;
    s->allow_voice_stealing = true;
    /* session->midi_io.synths[session->midi_io.num_synths] = s; */
    return s;
}

/* int synth_create_virtual_device(Synth *s) */
/* { */
/*     if (call_count > 0) { */
/* 	fprintf(stderr, "CALL COUNT!\n"); */
/* 	exit(1); */
/*     } */
/*     call_count++; */
/*     static const char *device_name = "Jackdaw Synth"; */

/*     PmError err = Pm_CreateVirtualInput( */
/* 	device_name, */
/* 	MIDI_INTERFACE_NAME, */
/* 	NULL); */
/*     if (err  < 0) { */
/* 	fprintf(stderr, "Error creating virtual device: %s\n", Pm_GetErrorText(err)); */
/* 	exit(1); */
/*     } */
/*     /\* if (err == pmInterfaceNotSupported) { *\/ */
/*     /\* 	fprintf(stderr, "Error: Interface \"%s\" not supported\n", MIDI_INTERFACE_NAME); *\/ */
/*     /\* 	return -1; *\/ */
/*     /\* } else if (err == pmInvalidDeviceId) { *\/ */
/*     /\* 	fprintf(stderr, "Error: device name \"%s\" invalid or already in use\n", device_name); *\/ */
/*     /\* 	return -1; *\/ */
/*     /\* } else  *\/ */
/*     /\* PmDeviceID id = err; *\/ */
/*     /\* s->device.id = id; *\/ */
/*     /\* s->device.info = Pm_GetDeviceInfo(id); *\/ */
    
/*     /\* err = Pm_OpenInput(&s->device.stream, s->device.id, NULL, PM_EVENT_BUF_NUM_EVENTS, NULL, NULL); *\/ */
/*     if (err != pmNoError) { */
/* 	fprintf(stderr, "ERROR OPENING DEVICE: %s\n", Pm_GetErrorText(err)); */
/* 	exit(1); */
/*     } */
    
/*     return id; */
/* } */

static float osc_sample(Osc *osc, int channel, int num_channels, float step)
{
    float sample;
    switch(osc->type) {
    case WS_SINE:
	sample = sin(TAU * osc->phase[channel]);
	break;
    case WS_SQUARE:
	sample = osc->phase[channel] < 0.5 ? -1.0 : 1.0;
	break;
    case WS_TRI:
	sample =
	    osc->phase[channel] <= 0.25 ?
	        osc->phase[channel] * 4.0f :
	    osc->phase[channel] <= 0.5f ?
	        (0.5f - osc->phase[channel]) * 4.0 :
	    osc->phase[channel] <= 0.75f ?
	        (osc->phase[channel] - 0.5f) * -4.0 :
	        (1.0f - osc->phase[channel]) * -4.0;
        break;
    case WS_SAW:
	sample = 2.0f * osc->phase[channel] - 1.0;
	break;
    default:
	sample = 0.0;
    }
    osc->phase[channel] += osc->sample_phase_incr * step;
    if (osc->phase[channel] > 1.0) {
	osc->phase[channel] -= 1.0;
    }
    float pan_scale;
    if (channel == 0) {
	pan_scale = osc->pan <= 0.5 ? 1.0 : (1.0f - osc->pan) * 2.0f;
    } else {
	pan_scale = osc->pan >= 0.5 ? 1.0 : osc->pan * 2.0f;
    }
    return sample * osc->amp * pan_scale;
}


static void synth_voice_add_buf(SynthVoice *v, float *buf, int32_t len, int channel, float step)
{
    if (v->available) return;
    /* fprintf(stderr, "\tvoice %p has data\n", v); */
    for (int i=0; i<v->synth->num_oscs; i++) {
	Osc *osc = v->oscs + i;
	/* fprintf(stderr, "\t\tosc %d, start rel %d end rel %d\n", i, v->note_start_rel, v->note_end_rel); */
	/* int32_t start_rel = v->note_start_rel; */
	/* int32_t end_rel = v->note_end_rel; */
	for (int i=0; i<len; i++) {
	    float sample = osc_sample(osc, channel, 2, step);
	    float env = 1.0f;
	    if (v->note_end_rel[channel] == 0) {
		v->amp_env_stage[channel] = 3;
		v->release_start_env[channel] = v->last_env[channel];
		v->env_remaining[channel] = v->synth->amp_env.r;
	    }
	    if (v->note_start_rel[channel] > 0) {
		v->note_start_rel[channel]++;
		continue;
	    }
	    
	    if (v->env_remaining[channel] == 0) {
		v->amp_env_stage[channel]++;
		switch (v->amp_env_stage[channel]) {
		case 1:
		    v->env_remaining[channel] = v->synth->amp_env.d;
		    break;
		case 2:
		    v->env_remaining[channel] = -1;
		    env = v->synth->amp_env.s;
		    break;
		case 3:
		    v->env_remaining[channel] = v->synth->amp_env.r;
		    break;
		case 4:
		    v->available = true;
		    return; /* We've passed ADSR */
		}		
	    }
	    switch(v->amp_env_stage[channel]) {
	    case 0:
		env = (float)(v->synth->amp_env.a - v->env_remaining[channel]) / v->synth->amp_env.a;
		break;
	    case 1:
		/* env = v->synth->amp_env.s + (float)(v->env_remaining[channel]) / v->synth->amp_env.d; */
		env = v->synth->amp_env.s + ((float)v->env_remaining[channel] / v->synth->amp_env.d) * (1.0f - v->synth->amp_env.s);
		break;
	    case 2:
		env = v->synth->amp_env.s;
		break;
	    case 3:
		env = v->release_start_env[channel] - v->release_start_env[channel] * (float)(v->synth->amp_env.r - v->env_remaining[channel]) / v->synth->amp_env.r;
		break;
	    }
	    sample *= env * (float)v->velocity / 127.0f;
 	    buf[i] += sample;
	    buf[i] = tanh(buf[i]);
	    v->last_env[channel] = env;
	    v->env_remaining[channel]--;
	    v->note_end_rel[channel]--;
	    v->note_start_rel[channel]--;
	    /* if (channel == 0) { */
	    /* 	fprintf(stderr, "Env stage %d, remaining %d\n", v->amp_env_stage[channel], v->env_remaining[channel]); */
	    /* } */
	}
	    /* if (start_rel < v->synth->amp_env.a) { */
	    /* 	/\* fprintf(stderr, "\t\t\t\tAttack!\n"); *\/ */
	    /* 	sample *= (float)start_rel / v->synth->amp_env.a; */
	    /* } else if (start_rel < v->synth->amp_env.a + v->synth->amp_env.d) { */
	    /* 	/\* fprintf(stderr, "\t\t\t\tDecay!\n"); *\/ */
	    /* 	sample *= v->synth->amp_env.s + (float)(start_rel - v->synth->amp_env.a) / v->synth->amp_env.d; */
	    /* } else if (end_rel < 0) { */
	    /* 	sample *= v->synth->amp_env.s; */
	    /* 	/\* fprintf(stderr, "\t\t\t\tSustain! * %f\n", v->synth->amp_env.s); *\/ */
	    /* } else if (end_rel < v->synth->amp_env.r) { */
	    /* 	/\* fprintf(stderr, "Num %d - %d\n", v->synth->amp_env.r, end_rel); *\/ */
	    /* 	sample *= v->synth->amp_env.s * (float)(v->synth->amp_env.r - end_rel) / v->synth->amp_env.r; */
	    /* 	/\* fprintf(stderr, "\t\t\t\tRelease!\n"); *\/ */
	    /* } else { */
	    /* 	v->available = true; */
	    /* 	/\* fprintf(stderr, "Passed ADSR!\n"); *\/ */
	    /* 	return; /\* We've passed ADSR *\/ */

	    /* } */
	    /* buf[i] += sample; */
	    /* start_rel++; */
	    /* end_rel++; */
	/* } */

	/* v->note_start_rel += len; */
	/* if (channel == 1) { */
	/*     v->note_start_rel += len; */
	/*     v->note_end_rel += len; */
	/* } */
    }
}

static void osc_set_freq(Osc *osc, double freq_hz)
{
    Session *session = session_get();
    osc->freq = freq_hz;
    osc->sample_phase_incr = freq_hz / session->proj.sample_rate;    
}
static void synth_voice_set_note(SynthVoice *v, uint8_t note_val, uint8_t velocity)
{
    v->note_val = note_val;
    for (int i=0; i<v->synth->num_oscs; i++) {
	/* osc_set_freq(v->oscs + i, MTOF[note_val]); */
	osc_set_freq(v->oscs + i, mtof_calc(note_val));
    }
    osc_set_freq(v->oscs + 1, mtof_calc((double)note_val + 0.02));
    fprintf(stderr, "OSC 1 %f OSC 2: %f\n", v->oscs[0].freq, v->oscs[1].freq);
    v->velocity = velocity;
}

static void synth_voice_assign_note(SynthVoice *v, double note, int velocity, int32_t start_rel)
{
    synth_voice_set_note(v, note, velocity);
    v->note_start_rel[0] = start_rel;
    v->note_start_rel[1] = start_rel;
    v->note_end_rel[0] = INT32_MIN;
    v->note_end_rel[1] = INT32_MIN;
    v->available = false;
    v->amp_env_stage[0] = 0;
    v->amp_env_stage[1] = 0;
    v->env_remaining[0] = v->synth->amp_env.a;
    v->env_remaining[1] = v->synth->amp_env.a;
    v->oscs[0].phase[0] = 0.0;
    v->oscs[0].phase[1] = 0.0;
    v->oscs[1].phase[0] = 0.0;
    v->oscs[1].phase[0] = 0.0;
    
    v->oscs[0].pan = 0.4;
    v->oscs[1].pan = 0.6;
    

}

void synth_feed_midi(Synth *s, PmEvent *events, int num_events, int32_t tl_start, bool send_immediate)
{
    /* FIRST: Update synth state from available MIDI data */
    for (int i=0; i<num_events; i++) {
	/* PmEvent e = s->device.buffer[i]; */
	PmEvent e = events[i];
	/* PmTimestamp ts = e.timestamp; */
	uint8_t status = Pm_MessageStatus(e.message);
	uint8_t note_val = Pm_MessageData1(e.message);
	uint8_t velocity = Pm_MessageData2(e.message);
	uint8_t msg_type = status;

	/* fprintf(stderr, "\t\tIN SYNTH msg%d/%d %x, %d, %d (%s)\n", i, s->num_events, status, note_val, velocity, msg_type == 0x80 ? "OFF" : msg_type == 0x90 ? "ON" : "ERROR"); */
	/* if (i == 0) start = e.timestamp; */
	/* int32_t pos_rel = ((double)e.timestamp - start) * (double)session->proj.sample_rate / 1000.0; */
	/* fprintf(stderr, "EVENT %d/%d, timestamp: %d (rel %d) pos rel %d (record start %d)\n", i, num_read, e.timestamp, current_time - e.timestamp, pos_rel, d->record_start); */
	if (velocity == 0) msg_type = 8;
	if (msg_type == 0x80) {
	    /* HANDLE NOTE OFF */
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		if (!v->available && v->note_val == note_val) {
		    v->note_end_rel[0] = 0;
		    v->note_end_rel[1] = 0;
		}
	    }
	} else if (msg_type == 0x90) {
	    bool note_assigned = false;
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		if (v->available) {
		    int32_t start_rel = send_immediate ? 0 : tl_start - e.timestamp;
		    synth_voice_assign_note(v, note_val, velocity, start_rel);
		    note_assigned = true;
		    break;
		}
	    }
	    if (!note_assigned) {
		fprintf(stderr, "No voices remaining! Voice stealing %d\n", s->allow_voice_stealing);
		if (s->allow_voice_stealing) {
		    int32_t note_start_rel = 0;
		    SynthVoice *oldest = NULL;
		    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
			SynthVoice *v = s->voices + i;
			if (v->note_start_rel[0] < note_start_rel) {
			    oldest = v;
			    note_start_rel = v->note_start_rel[0];
			}
		    }
		    if (oldest) {
			fprintf(stderr, "STOLE VOICE!\n");
			int32_t start_rel = send_immediate ? 0 : tl_start - e.timestamp;
			synth_voice_assign_note(oldest, note_val, velocity, start_rel);
			note_assigned = true;
		    }
		}
	    }
	}
    }
}

void synth_add_buf(Synth *s, float *buf, int channel, int32_t len, float step)
{
    if (step < 0.0) step *= -1;
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	synth_voice_add_buf(v, buf, len, channel, step);		
    }    
}


void synth_close_all_notes(Synth *s)
{
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	v->note_end_rel[0] = 0;
	v->note_end_rel[1] = 0;
    }
}
