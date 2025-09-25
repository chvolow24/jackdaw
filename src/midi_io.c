/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "clipref.h"
#include "midi_io.h"
#include "midi_clip.h"
#include "midi_objs.h"
#include "midi_qwerty.h"
#include "portmidi.h"
#include "porttime.h"
#include "session.h"

static int populate_global_midi_device_list(MIDIDevice *devices)
{
    int num_devices = Pm_CountDevices();
    for (int i=0; i<num_devices; i++) {
	MIDIDevice *d = devices + i;
	d->type = MIDI_DEVICE_PM;
	d->id = i;
	d->info = Pm_GetDeviceInfo(i);
	d->stream = NULL;
	d->latency = 0;
	d->name = d->info->name;
	d->input = d->info->input;
	d->output = d->info->output;
	d->opened = d->info->opened;
    }
    MIDIDevice *qwerty = devices + num_devices;
    qwerty->type = MIDI_DEVICE_QWERTY;
    qwerty->name = "QWERTY";
    qwerty->type = MIDI_DEVICE_QWERTY;
    qwerty->input = true;
    return num_devices + 1;
}

/* static int midi_device_create_jackdaw_out(MIDIDevice *dst) */
/* { */
/*     static const char *device_name = "Jackdaw MIDI out"; */
/*     PmError err = Pm_CreateVirtualOutput( */
/* 	device_name, */
/* 	MIDI_INTERFACE_NAME, */
/* 	NULL); */
/*     if (err == pmInterfaceNotSupported) { */
/* 	fprintf(stderr, "Error: Interface \"%s\" not supported\n", MIDI_INTERFACE_NAME); */
/* 	return -1; */
/*     } else if (err == pmInvalidDeviceId) { */
/* 	fprintf(stderr, "Error: device name \"%s\" invalid or already in use\n", device_name); */
/* 	return -1; */
/*     } */
/*     fprintf(stderr, "Create virtual in return code: %d\n", err); */
/*     PmDeviceID id = err; */
/*     dst->id = id; */
/*     dst->info = Pm_GetDeviceInfo(id); */
/*     return id; */
/* } */

/* static int midi_device_create_jackdaw_in(MIDIDevice *dst) */
/* { */
/*     static const char *device_name = "Jackdaw MIDI in"; */
/*     PmError err = Pm_CreateVirtualInput( */
/* 	device_name, */
/* 	MIDI_INTERFACE_NAME, */
/* 	NULL); */
/*     if (err == pmInterfaceNotSupported) { */
/* 	fprintf(stderr, "Error: Interface \"%s\" not supported\n", MIDI_INTERFACE_NAME); */
/* 	return -1; */
/*     } else if (err == pmInvalidDeviceId) { */
/* 	fprintf(stderr, "Error: device name \"%s\" invalid or already in use\n", device_name); */
/* 	return -1; */
/*     } */
/*     fprintf(stderr, "Create virtual in return code: %d\n", err); */
/*     PmDeviceID id = err; */
/*     dst->id = id; */
/*     dst->info = Pm_GetDeviceInfo(id); */
/*     return id; */
/* } */

int midi_io_init(void)
{

    PmError err = Pm_Initialize();
    if (err != pmNoError) {
	fprintf(stderr, "Error initializing PortMidi: %d\n", err);
    }
    
    return err;

    /* MIDIDevice jackdaw_midi_out, jackdaw_midi_in; */
}


void midi_io_deinit(void)
{
    PmError err = Pm_Terminate();
    if (err != pmNoError) {
	fprintf(stderr, "Error terminating PortMidi: %d\n", err);
    }
}

/* static int midi_create_virtual_devices(struct midi_io *midi_io) */
/* { */
/*     int ret = midi_device_create_jackdaw_out(&midi_io->out); */
/*     if (ret < 0) { */
/* 	fprintf(stderr, "Error creating jackdaw midi out\n"); */
/* 	return ret; */
/*     } */
    
/*     ret = midi_device_create_jackdaw_in(&midi_io->in); */

/*     if (ret < 0) { */
/* 	fprintf(stderr, "Error creating jackdaw midi in\n"); */
/* 	return ret; */
/*     } */
    
/*     PmError err = Pm_OpenInput(&midi_io->in.stream, midi_io->in.id, NULL, PM_EVENT_BUF_NUM_EVENTS, NULL, NULL); */
/*     if (err != pmNoError) { */
/* 	fprintf(stderr, "Error opening jackdaw midi in! %d\n", err); */
/*     } */
/*     err = Pm_OpenOutput(&midi_io->out.stream, midi_io->out.id, NULL, PM_EVENT_BUF_NUM_EVENTS, NULL, NULL, 0); */
/*     if (err != pmNoError) { */
/* 	fprintf(stderr, "Error opening jackdaw midi out! %d\n", err); */
/*     } */
/*     return ret; */
/* } */

/* static void midi_close_virtual_devices(struct midi_io *midi_io) */
/* { */
/*     PmError err = Pm_Close(midi_io->in.stream); */
/*     if (err != pmNoError) { */
/* 	fprintf(stderr, "Error closing input device: %s\n", Pm_GetErrorText(err)); */
/*     } */
/*     err = Pm_Close(midi_io->out.stream); */
/*     if (err != pmNoError) { */
/* 	fprintf(stderr, "Error closing output device: %s\n", Pm_GetErrorText(err)); */
/*     } */

/* } */


/* static void open_output(MIDIDevice *device) */
/* { */
/*     PmError err = Pm_OpenOutput(&device->stream, device->id, NULL, PM_EVENT_BUF_NUM_EVENTS, NULL, NULL, MIDI_OUTPUT_LATENCY); */
/*     if (err != pmNoError) fprintf(stderr, "Error opening output device, code %d: %s\n", err, Pm_GetErrorText(err)); */
/*     else fprintf(stderr, "Successfully opened %s\n", device->info->name); */
/* } */

/* int pop_list_calls = 0; */

void session_populate_midi_device_lists(Session *session)
{

    /* if (pop_list_calls != 0) { */
    /* 	fprintf(stderr, "panic panic panic!\n"); */
    /* 	exit(1); */
    /* } */
    /* pop_list_calls++; */
    /* PortMidi will only know about newly-connected devices when
       Pm_Initialize is called again :( */   
    /* midi_close_virtual_devices(&session->midi_io); */
    /* Pm_Terminate(); */
    /* Pm_Initialize(); */
    /* midi_create_virtual_devices(&session->midi_io); */
    
    session->midi_io.num_inputs = 0;
    session->midi_io.num_outputs = 0;
    MIDIDevice devices[MAX_MIDI_DEVICES * 2];
    memset(devices, '\0', sizeof(devices));
    int num = populate_global_midi_device_list(devices);
    fprintf(stderr, "found %d\n", num);
    if (num < 0) {
	fprintf(stderr, "Error collecting midi device list. Errno: %d\n", num);
    } else {
	for (int i=0; i<num; i++) {
	    MIDIDevice *device = devices + i;
	    if (device->input) {
		session->midi_io.inputs[session->midi_io.num_inputs] = *device;
		if (device->type == MIDI_DEVICE_PM) {
		    midi_device_open(session->midi_io.inputs + session->midi_io.num_inputs);
		} else if (device->type == MIDI_DEVICE_QWERTY) {
		    session->midi_io.midi_qwerty = session->midi_io.inputs + session->midi_io.num_inputs;
		}
		session->midi_io.num_inputs++;
	    } else {
		session->midi_io.outputs[session->midi_io.num_outputs] = *device;
		session->midi_io.num_outputs++;
		/* if (strcmp(device->info->name, "Nord Piano 5 MIDI Input") == 0) { */
		/*     fprintf(stderr, "^^^^opening device Nord Piano 5\n"); */
		/*     session->midi_io.primary_output = session->midi_io.outputs + session->midi_io.num_outputs - 1; */
		/*     open_output(session->midi_io.primary_output); */
		/* } */
	    }
	}
    }
    /* synth_create_virtual_device(&session->synth); */
    /* synth_init_defaults(&session->synth); */


}

int session_init_midi(Session *session)
{
    /* int ret = midi_create_virtual_devices(&session->midi_io); */
    session_populate_midi_device_lists(session);
    return 0;
}

void session_deinit_midi(Session *session)
{
    /* midi_close_virtual_devices(&session->midi_io); */
}

int midi_device_open(MIDIDevice *d)
{
    switch (d->type) {
    case MIDI_DEVICE_PM:
	if (!d->info) {
	    fprintf(stderr, "Error: cannot open device. 'info' is missing.\n");
	    return -1;
	}
	if (d->info->opened) {
	    fprintf(stderr, "Device already open\n");
	    return 0;
	}
	PmError err = pmNoError;
	if (d->info->input) {
	    Pm_OpenInput(
		&d->stream,
		d->id,
		NULL,
		PM_EVENT_BUF_NUM_EVENTS,
		NULL,
		NULL);
	    d->record_start = Pt_Time();
	} else {
	    Pm_OpenOutput(
		&d->stream,
		d->id,
		NULL,
		PM_EVENT_BUF_NUM_EVENTS,
		NULL,
		NULL,
		0); /* TODO: figure out latency */

	}
	d->opened = d->info->opened;
	/* fprintf(stderr, "OPENED DEVICE %p %s -- %d -- %p\n", d, d->info->name, d->info->opened, d->stream); */
	if (err != pmNoError) {
	    fprintf(stderr, "Error opening device %s: %s\n", d->info->name, Pm_GetErrorText(err));
	}
	return err;

	break;
    case MIDI_DEVICE_QWERTY: {
	d->opened = true;
	d->num_unconsumed_events = 0;
    }
	break;
    default:
	break;
    }
    return 0;

}

int midi_device_close(MIDIDevice *d)
{
    switch (d->type) {
    case MIDI_DEVICE_PM:
	if (!d->info->opened) return 0;
	PmError err = Pm_Close(d->stream);
	if (err != pmNoError) {
	    fprintf(stderr, "Error closing device %s: %s\n", d->info->name, Pm_GetErrorText(err));
	}
	d->opened = d->info->opened;
	return err;
    case MIDI_DEVICE_QWERTY:
	d->opened = false;
	d->num_unconsumed_events = 0;
	break;
    default:
	break;
    }
    return 0;
}

/* Returns number of unconsumed events, or 0 if buf full */
int midi_device_add_event(MIDIDevice *d, PmEvent e)
{
    if (d->num_unconsumed_events >= PM_EVENT_BUF_NUM_EVENTS) {
	return 0;
    }
    d->buffer[d->num_unconsumed_events] = e;
    d->num_unconsumed_events++;
    return d->num_unconsumed_events;
}

/* Get MIDI data from the source device and add it to the devie event buf */
void midi_device_read(MIDIDevice *d)
{
    if (d->type == MIDI_DEVICE_QWERTY) {
	mqwert_get_current_notes(d);
	return;
    }
    /* fprintf(stderr, "opened? %d\n", d->info->opened); */
    if (!d->info || !d->info->opened) return;

    if (d->num_unconsumed_events > 0) {
	fprintf(stderr, "Error: overwriting unconsumed events on device \"%s\"\n", d->info->name);
    }
    /* Session *session = session_get(); */
    /* Timeline *tl = ACTIVE_TL; */
    int num_read = Pm_Read(
	d->stream,
	d->buffer,
	sizeof(d->buffer) / sizeof(PmEvent));

    if (num_read < 0) {
	fprintf(stderr, "Error: midi record error: %s\n", Pm_GetErrorText(num_read));
	fprintf(stderr, "Args passed: device: %p, d->stream %p, d->buffer %p\n", d, d->stream, d->buffer);
	Session *session = session_get();
	for (int i=0; i<session->midi_io.num_inputs; i++) {
	    MIDIDevice *dl = session->midi_io.inputs + i;
	    fprintf(stderr, "INPUT %d/%d: %p, %s stream %p", i, session->midi_io.num_inputs, dl, dl->info->name, dl->stream);
	    if (dl == d) {
		fprintf(stderr, " <---\n");
	    } else {
		fprintf(stderr, "\n");
	    }
	}
	/* exit(1); */
	return;
    }
    d->num_unconsumed_events = num_read;
    /* PmTimestamp current_time = Pt_Time(); */
    /* for (int i=0; i<num_read; i++) { */
    /* 	PmEvent e = d->buffer[i]; */
    /* 	uint8_t status = Pm_MessageStatus(e.message); */
    /* 	uint8_t note_val = Pm_MessageData1(e.message); */
    /* 	uint8_t velocity = Pm_MessageData2(e.message); */
    /* 	uint8_t msg_type = status >> 4; */
    /* 	/\* uint8_t channel = (status & 0xF); *\/ */
    /* 	/\* fprintf(stderr, "\t%s %d velocity %d channel %d\n", type_name, data1, data2, channel); *\/ */
    /* 	int32_t pos_rel = ((double)e.timestamp - d->record_start) * (double)session_get_sample_rate() / 1000.0; */
    /* 	/\* fprintf(stderr, "EVENT %d/%d, timestamp: %d (rel %d) pos rel %d (record start %d)\n", i, num_read, e.timestamp, current_time - e.timestamp, pos_rel, d->record_start); *\/ */
    /* 	if (msg_type == 9 && d->current_clip) { */
    /* 	    Note *unclosed = d->unclosed_notes + note_val; */
    /* 	    unclosed->note = note_val; */
    /* 	    unclosed->velocity = velocity; */
    /* 	    unclosed->start_rel = pos_rel; */
    /* 	    /\* fprintf(stderr, "\tadding unclosed pitch %d\n", unclosed->note); *\/ */
    /* 	} else if (msg_type == 8 && d->current_clip) { */
    /* 	    Note *unclosed = d->unclosed_notes + note_val; */
    /* 	    midi_clip_add_note(d->current_clip, note_val, unclosed->velocity, unclosed->start_rel, pos_rel); */
    /* 	} */
    /* } */
}

/* Add device events to clip */
void midi_device_output_chunk_to_clip(MIDIDevice *d, enum midi_ts_type ts_type)
{
    /* fprintf(stderr, "Current clip? %p\n", d->current_clip); */
    if (!d->current_clip) return;
    /* PmTimestamp current_time = Pt_Time(); */
    /* fprintf(stderr, "PROCESSING %d unconsumed...\n", d->num_unconsumed_events); */
    for (int i=0; i<d->num_unconsumed_events; i++) {
	PmEvent e = d->buffer[i];
	uint8_t status = Pm_MessageStatus(e.message);
	uint8_t channel = status & 0x0F;
	uint8_t note_val = Pm_MessageData1(e.message);
	uint8_t velocity = Pm_MessageData2(e.message);
	uint8_t msg_type = status >> 4;
	int32_t pos_rel;
	if (ts_type == MIDI_TS_SFRAMES) {
	    pos_rel = e.timestamp;
	} else if (ts_type == MIDI_TS_MSEC) { /* MSEC */
	    pos_rel = ((double)e.timestamp - d->record_start) * (double)session_get_sample_rate() / 1000.0;
	} else {
	    return;
	}
	/* fprintf(stderr, "EVENT %d/%d, timestamp: %d pos rel %d (record start %d)\n", i, d->num_unconsumed_events, e.timestamp, pos_rel, d->record_start); */
	if (msg_type == 9 && d->current_clip) {
	    Note *unclosed = d->unclosed_notes + note_val;
	    unclosed->key = note_val;
	    unclosed->velocity = velocity;
	    unclosed->start_rel = pos_rel;
	    unclosed->unclosed = true;
	} else if (msg_type == 8 && d->current_clip) {
	    /* fprintf(stderr, "Got a note off\n"); */
	    Note *unclosed = d->unclosed_notes + note_val;
	    /* if (d->current_clip) */
	    if (unclosed->unclosed) {
		midi_clip_insert_note(d->current_clip, channel, note_val, unclosed->velocity, unclosed->start_rel, pos_rel);
		unclosed->unclosed = false;
	    }
	} else if (msg_type == 0xB && d->current_clip) { /* Controller */
	    midi_clip_add_controller_change(d->current_clip, e, pos_rel);
	    /* MIDICC cc = midi_cc_from_event(&e, pos_rel); */
	    /* midi_clip_add_cc(d->current_clip, cc); */
	} else if (msg_type == 0xE) {
	    /* fprintf(stderr, "RECORD PITCH BEND!\n"); */
	    midi_clip_add_pitch_bend(d->current_clip, e, pos_rel);
	    /* MIDIPitchBend pb = midi_pitch_bend_from_event(&e, pos_rel); */
	    /* midi_clip_add_pb(d->current_clip, pb); */
	}
    }    
}

/* Uses PortTime timer for end rel timestamp */
void midi_device_close_all_notes(MIDIDevice *d)
{
    /* fprintf(stderr, "Closing all notes, then current clip %p\n", d->current_clip); */
    for (int i=0; i<128; i++) {
	Note *n = d->unclosed_notes + i;
	if (n->unclosed && d->current_clip) {
	    PmEvent e;
	    e.timestamp = Pt_Time();
	    e.message = Pm_Message(
		0x80,
		n->key,
		0);
	    if (midi_device_add_event(d, e) == 0) {
		fprintf(stderr, "Error: no room for note off in midi_device_close_all_notes\n");
	    }
	    /* n->unclosed = false; */
	}
    }
    if (d->current_clip) {
 	midi_device_output_chunk_to_clip(d, MIDI_TS_MSEC);
    }
}


static void track_flush_unclosed_midi_notes(Track *track)
{
    if (!track->midi_out) return;
    switch(track->midi_out_type) {
    case MIDI_OUT_DEVICE:
	break;
    case MIDI_OUT_SYNTH:
	synth_close_all_notes(track->midi_out);
	break;
	
    }
    track->note_offs.read_i = 0;
    track->note_offs.num_queued = 0;
}
void timeline_flush_unclosed_midi_notes()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    for (int i=0; i<tl->num_tracks; i++) {
	Track *track = tl->tracks[i];
	track_flush_unclosed_midi_notes(track);	
    }
}
