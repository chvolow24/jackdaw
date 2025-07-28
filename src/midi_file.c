#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clipref.h"
#include "midi_clip.h"
#include "midi_io.h"
#include "midi_objs.h"
#include "portmidi.h"
#include "prompt_user.h"
#include "session.h"
#include "tempo.h"
#include "type_serialize.h"


/* Spec at https://drive.google.com/file/d/1t4jcCCKoi5HMi7YJ6skvZfKcefLhhOgU/view?u */

/* MIDI file metadata is captured in static global variables during file read.
   The only accessible function in this file is midi_file_open. */

typedef enum {
    MIDI_CHUNK_UNKNOWN,
    MIDI_CHUNK_HDR,
    MIDI_CHUNK_TRCK
} MIDIChunkType;

struct division_fmt {
    uint8_t fmt_bit; /* 0 for ticks per quarter, 1 for SMPTE FPS */
    uint16_t ticks_per_quarter;
    int8_t smpte_fmt;
    uint8_t ticks_per_frame;
    double sample_frames_per_division; /* OUTPUT after tempo */
};

struct midi_time_sig {
    uint8_t num;
    uint8_t denom;
    uint8_t midi_clocks_per_click;
    uint8_t thirty_second_notes_per_clock;
};

struct midi_key_sig {
    int8_t num_sharps_or_flats; /* flats if minor */
    uint8_t is_minor;
};

struct midi_file {
    const char *path;
    uint16_t format;
    uint16_t num_tracks;
    uint16_t division_raw;
    struct division_fmt division_fmt;
    struct midi_time_sig time_sig;
    struct midi_key_sig key_sig;
};




/* Translation unit state */

static struct midi_file file_info;
ClickTrack *click_track;
ClickSegment *click_segment;

/* "Virtual" devices shoehorned into this process due to pre-existence of
   midi_device_record_chunk */
static MIDIDevice *v_device;
static int channel_name_index = 0;

static int channel_instruments[16] = {0};

const char *MIDI_PC_INSTRUMENT_NAMES[] = {
    "Acoustic grand piano",
    "Bright acoustic piano",
    "Electric grand piano",
    "Honky tonk piano",
    "Electric piano 1",
    "Electric piano 2",
    "Harpsicord",
    "Clavinet",
    "Celesta",
    "Glockenspiel",
    "Music box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular bell",
    "Dulcimer",
    "Hammond / drawbar organ",
    "Percussive organ",
    "Rock organ",
    "Church organ",
    "Reed organ",
    "Accordion",
    "Harmonica",
    "Tango accordion",
    "Nylon string acoustic guitar",
    "Steel string acoustic guitar",
    "Jazz electric guitar",
    "Clean electric guitar",
    "Muted electric guitar",
    "Overdriven guitar",
    "Distortion guitar",
    "Guitar harmonics",
    "Acoustic bass",
    "Fingered electric bass",
    "Picked electric bass",
    "Fretless bass",
    "Slap bass 1",
    "Slap bass 2",
    "Synth bass 1",
    "Synth bass 2",
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo strings",
    "Pizzicato strings",
    "Orchestral strings / harp",
    "Timpani",
    "String ensemble 1",
    "String ensemble 2 / slow strings",
    "Synth strings 1",
    "Synth strings 2",
    "Choir aahs",
    "Voice oohs",
    "Synth choir / voice",
    "Orchestra hit",
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted trumpet",
    "French horn",
    "Brass ensemble",
    "Synth brass 1",
    "Synth brass 2",
    "Soprano sax",
    "Alto sax",
    "Tenor sax",
    "Baritone sax",
    "Oboe",
    "English horn",
    "Bassoon",
    "Clarinet",
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan flute",
    "Bottle blow / blown bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",
    "Synth square wave",
    "Synth saw wave",
    "Synth calliope",
    "Synth chiff",
    "Synth charang",
    "Synth voice",
    "Synth fifths saw",
    "Synth brass and lead",
    "Fantasia / new age",
    "Warm pad",
    "Polysynth",
    "Space vox / choir",
    "Bowed glass",
    "Metal pad",
    "Halo pad",
    "Sweep pad",
    "Ice rain",
    "Soundtrack",
    "Crystal",
    "Atmosphere",
    "Brightness",
    "Goblins",
    "Echo drops / echoes",
    "Sci fi",
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bag pipe",
    "Fiddle",
    "Shanai",
    "Tinkle bell",
    "Agogo",
    "Steel drums",
    "Woodblock",
    "Taiko drum",
    "Melodic tom",
    "Synth drum",
    "Reverse cymbal",
    "Guitar fret noise",
    "Breath noise",
    "Seashore",
    "Bird tweet",
    "Telephone ring",
    "Helicopter",
    "Applause",
    "Gunshot"
};


/* Utils for deserializing values */

static uint32_t get_variable_length(FILE *file, int *num_bytes_dst)
{
    uint32_t value = 0;
    uint8_t byte;
    int num_bytes = 0;
    do {
        byte = fgetc(file);
        value = (value << 7) | (byte & 0x7F);
	num_bytes++;
    } while (byte & 0x80 && num_bytes < 4);

    if (num_bytes_dst) *num_bytes_dst = num_bytes;
    return value;
}

static uint32_t get_32(FILE *f)
{
    uint8_t bytes[4];
    fread(bytes, 1, 4, f);
    uint32_t ret = (uint32_t)bytes[0] << 24;
    ret += (uint32_t)bytes[1] << 16;
    ret += (uint32_t)bytes[2] << 8;
    ret += (uint32_t)bytes[3];
    return ret;    
}

static uint32_t get_24(FILE *f)
{
    uint8_t bytes[3];
    fread(bytes, 1, 3, f);
    uint32_t ret = (uint32_t)bytes[0] << 16;
    ret += (uint32_t)bytes[1] << 8;
    ret += bytes[2];
    return ret;
}

static uint16_t get_16(FILE *f)
{
    uint8_t bytes[2];
    fread(bytes, 1, 2, f);
    uint16_t ret = (uint16_t)bytes[0] << 8;
    ret += bytes[1];
    return ret;
}


static MIDIChunkType get_chunk_data(FILE *f, uint32_t *len)
{
    MIDIChunkType t = MIDI_CHUNK_UNKNOWN;
    char hdr[5];
    fread(hdr, 1, 4, f);
    if (memcmp(hdr, "MThd", 4) == 0) {
	t = MIDI_CHUNK_HDR;
    } else if (memcmp(hdr, "MTrk", 4) == 0) {
	t = MIDI_CHUNK_TRCK;
    } else {
	hdr[4] = '\0';
	fprintf(stderr, "Unknown hdr: %s\n", hdr);
    }
    *len = get_32(f);
    return t;
}

static int8_t twos_comp_decode(uint8_t byte)
{
    uint8_t negative = byte >> 7;
    int8_t number = byte;
    if (negative) {
	byte--;
	byte = ~byte; /* flip the bits */
	number = byte * -1;
    }
    return number;
}

static int8_t decode_smpte_fps(uint8_t raw)
{
    uint8_t val7 = raw & 0x7F;
    uint8_t val8 = val7;

    if (val7 & 0b01000000) { /* convert to 8-bit twos comp */
	val8 &= ~0b01000000; /* unset bit 6 */
	val8 |= 0b10000000; /* set bit 7 instead */
    }
    return twos_comp_decode(val8);
}

/* Assumes already read "MThd" and length value */
static void get_midi_hdr(FILE *f)
{
    file_info.format = get_16(f);
    
    file_info.num_tracks = get_16(f);
    uint16_t division = get_16(f);
    file_info.division_raw = division;
    uint8_t division_msb = division >> 15;
    
    file_info.division_fmt.fmt_bit = division_msb;
    
    uint16_t rest = division & 0x7FFF;
    /* fprintf(stderr, "\tmsb == %d\n", division_msb); */
    if (division_msb == 0) {
	file_info.division_fmt.ticks_per_quarter = rest;	
    } else { /* SMPTE */
	/* TODO: handle smpte format */
	uint8_t smpte_byte = rest >> 8;
	file_info.division_fmt.smpte_fmt = decode_smpte_fps(smpte_byte);
	file_info.division_fmt.ticks_per_frame = rest & 0xFF;
    }
}

/* Assumes already read "MTrk" and length value */
static void get_midi_trck(FILE *f, int32_t len, int track_index, MIDIClip **mclips, int num_clips, int32_t tl_start_pos)
{
    fprintf(stderr, "\n\n\n");
    uint32_t num_note_ons[16] = {0};
    uint32_t num_note_offs[16] = {0};
    uint64_t running_ts = 0;
    
    bool track_ended = false;
    while (!track_ended && len > 0) {
	PmEvent e;
	int num_bytes;
	uint32_t delta_time = get_variable_length(f, &num_bytes);

	running_ts += delta_time * (uint32_t)file_info.division_fmt.sample_frames_per_division;
	e.timestamp = running_ts;
	len -= num_bytes;	
	uint8_t status = fgetc(f);
	len--;
	static uint8_t prev_status;
    done_get_status:
	(void)0;
	uint8_t channel = status & 0x0F;
	if (status == 0xFF) { /* META EVENT */
	    uint8_t type = fgetc(f);
	    uint32_t length = get_variable_length(f, &num_bytes);
	    char *buf = malloc(length + 1);
	    switch (type) {
	    case 0x00: {
		uint16_t sequence_number = get_16(f);
		fprintf(stderr, "Sequence number: %d\n", sequence_number);
	    }
		break;
	    case 0x01:		
		fread(buf, 1, length, f);
		buf[length] = '\0';
		fprintf(stderr, "Text event: \"%s\"\n", buf);
		if (channel_name_index >= num_clips) break;
		strncpy(mclips[channel_name_index]->name, buf, length + 1);
		channel_name_index++;
		break;
	    case 0x02:
		fprintf(stderr, "Copyright notice\n");
		break;
	    case 0x03:
		fread(buf, 1, length, f);
		buf[length] = '\0';
		if (file_info.format == 1 && track_index > 0) {
		    mclips[track_index - 1]->midi_track_name = strndup(buf, MAX_NAMELENGTH);
		    fprintf(stderr, "ASSIGNING Track name to clip index %d: \"%s\"\n", track_index - 1, buf);
		}
		break;
	    case 0x04: {
		fread(buf, 1, length, f);
		buf[length] = '\0';
		fprintf(stderr, "Instrument name: \"%s\"\n", buf);
		/* char buf2[length + 3]; */
		/* memcpy(buf2, buf + 3, length); */
		/* buf2[0] = ' '; */
		/* buf2[1] = '-'; */
		/* buf2[2] = ' '; */
		/* strncat(mclips[track_index - 1]->name, buf2, length + 3); */
	    }
		break;
	    case 0x20: {
		uint16_t channel_prefix = get_16(f);
		fprintf(stderr, "Channel prefix: %d\n", channel_prefix);
	    }
		break;
	    case 0x2F: {
		fprintf(stderr, "END OF TRACK\n");
		track_ended = true;
	    }
		break;
	    case 0x51: {
		uint32_t tempo_us_per_quarter = get_24(f);
		double bpm = 1000000.0 * 60.0 / (double)tempo_us_per_quarter;
		fprintf(stderr, "BPM: %f\n", bpm);
		ClickSegment *cs = click_track_cut_at(click_track, e.timestamp + tl_start_pos);
		if (!cs) cs = click_track_get_segment_at_pos(click_track, e.timestamp + tl_start_pos);
		uint8_t subdivs[4] = {4, 4, 4, 4};
		click_segment_set_config(cs, -1, bpm, 4, subdivs, 0);
		/* exit(0); */
		double us_per_tick = (double)tempo_us_per_quarter / (double)file_info.division_fmt.ticks_per_quarter;
		double frames_per_tic = us_per_tick * session_get_sample_rate() / 1000000.0;
		file_info.division_fmt.sample_frames_per_division = frames_per_tic;
		fprintf(stderr, "US_PER_QUARTER: %ud, FRAMES PER TICK: %f\n", tempo_us_per_quarter, frames_per_tic);
		/* exit(0); */
	    }
		break;
	    case 0x58: {
		uint8_t num = fgetc(f);
		uint8_t denom = fgetc(f);
		uint8_t midi_clocks_per_click = fgetc(f);
		uint8_t thirty_second_notes_per_clock = fgetc(f);
		fprintf(stderr, "TIME SIG: %d/%d, %d; %d\n", num, denom, midi_clocks_per_click, thirty_second_notes_per_clock);
	    }
		break;
	    case 0x59: {
		int8_t sf = fgetc(f);
		uint8_t mi = fgetc(f);
		fprintf(stderr, "KEY SIG: %d %s, %s\n ", abs(sf), sf < 0 ? "flats" : "sharps", mi ? "(minor)" : "(major)");			
	    }
		break;
	    default:
		/* fprintf(stderr, "Ignore meta event type %x (length %d)\n", type, length);		 */
		fseek(f, length, SEEK_CUR); /* IGNORE META EVENT */				    
	    }
	    free(buf);
	    len -= (num_bytes + length + 1);
	    
	} else if ((status & 0xF0) == 0x80) { /* NOTE OFF */
	    uint8_t note = fgetc(f);
	    uint8_t velocity = fgetc(f);
	    if (channel == 3) {
		fprintf(stderr, "(%d) NOTE OFF (%d, vel %d)\n", e.timestamp, note, velocity);
	    }
	    num_note_offs[channel]++;
	    uint8_t clip_index = file_info.format == 0 ? channel : track_index - 1;
	    /* fprintf(stderr, "\t\t(off) Channel %d note %d vel %d\n", channel, note, velocity); */
	    len -= 2;
	    if (clip_index >= num_clips) {
		break;
	    }
	    e.message = Pm_Message(status, note, velocity);
	    v_device[clip_index].buffer[v_device[clip_index].num_unconsumed_events] = e;
	    v_device[clip_index].num_unconsumed_events++;
	    if (v_device[clip_index].num_unconsumed_events == PM_EVENT_BUF_NUM_EVENTS) {
		midi_device_record_chunk(&v_device[clip_index], 0);
		v_device[clip_index].num_unconsumed_events = 0;
	    }

	} else if ((status & 0xF0) == 0x90) { /* NOTE ON */
	    uint8_t note = fgetc(f);
	    uint8_t velocity = fgetc(f);
	    if (channel == 3) {
		fprintf(stderr, "(%d) NOTE ON (%d)\n", e.timestamp, note);
		if (velocity == 0) {
		    fprintf(stderr, "\t\t(go note off)\n");
		}
	    }
	    if (velocity == 0) { /* Actually a note off */
		num_note_offs[channel]++;
		uint8_t clip_index = file_info.format == 0 ? channel : track_index - 1;
		len -= 2;
		if (clip_index >= num_clips) {
		    break;
		}
		/* Convert type to note off for internal use */
		e.message = Pm_Message(0x80 + channel, note, velocity);
		v_device[clip_index].buffer[v_device[clip_index].num_unconsumed_events] = e;
		v_device[clip_index].num_unconsumed_events++;
		if (v_device[clip_index].num_unconsumed_events == PM_EVENT_BUF_NUM_EVENTS) {
		    midi_device_record_chunk(&v_device[clip_index], 0);
		    v_device[clip_index].num_unconsumed_events = 0;
		}
	    } else { /* Authentic note on */
		num_note_ons[channel]++;
		uint8_t clip_index = file_info.format == 0 ? channel : track_index - 1;
		/* fprintf(stderr, "\t\tChannel %d note %d vel %d\n", channel, note, velocity); */
		len -= 2;
		if (clip_index >= num_clips) {
		    break;
		}

		e.message = Pm_Message(status, note, velocity);
		v_device[clip_index].buffer[v_device[clip_index].num_unconsumed_events] = e;
		v_device[clip_index].num_unconsumed_events++;
		if (v_device[clip_index].num_unconsumed_events == PM_EVENT_BUF_NUM_EVENTS) {
		    midi_device_record_chunk(&v_device[clip_index], 0);
		    v_device[clip_index].num_unconsumed_events = 0;
		}
	    }
	} else {
	    /* fprintf(stderr, "IN SWITCH x 0xF0: %x\n", status & 0xF0); */
	    switch (status & 0xF0) {
	    case 0xA0: // Aftertouch
		fgetc(f); fgetc(f);
		len -= 2;
		break;
	    case 0xB0: { /* Controller */
		uint8_t data1 = fgetc(f);
		uint8_t data2 = fgetc(f);
		if (data1 == 0) {
		    fprintf(stderr, "(%llu) BANK SELECT channel %d DATA: %d\n", running_ts, channel, data2);
		}
		len -= 2;
		/* fprintf(stderr, "CONTROL: %d %d\n", fgetc(f), fgetc(f)); */
	    }
		break;

	    case 0xE0: {// Pitch bend
		uint8_t lsb = fgetc(f);
		uint8_t msb = fgetc(f);
		/* float floatval = (float)val / 16384.0f; */
		/* MIDIPitchBend pb = midi_pitch_bend_from_event(&e, 0); */
		uint8_t clip_index = file_info.format == 0 ? channel : track_index - 1;
		e.message = Pm_Message(status, lsb, msb);
		v_device[clip_index].buffer[v_device[clip_index].num_unconsumed_events] = e;
		v_device[clip_index].num_unconsumed_events++;
		if (v_device[clip_index].num_unconsumed_events == PM_EVENT_BUF_NUM_EVENTS) {
		    midi_device_record_chunk(&v_device[clip_index], 0);
		    v_device[clip_index].num_unconsumed_events = 0;
		}

		/* fgetc(f); fgetc(f); */
		len -= 2;
	    }
		break;
	    case 0xC0: { // Program change
		int pc_data = fgetc(f);
		if (pc_data < sizeof(MIDI_PC_INSTRUMENT_NAMES) / sizeof(char *)) {
		    if (file_info.format == 1) {
			MIDIClip *mclip = mclips[track_index - 1];
			if (mclip && !mclip->primary_instrument_name) {
			    mclip->primary_instrument = pc_data;
			    mclip->primary_instrument_name = MIDI_PC_INSTRUMENT_NAMES[pc_data];
			}
		    } else if (file_info.format == 0) {
			/* int channel = status & 0x0F; */
			/* if (channel_instruments_index < 16) { */
			channel_instruments[channel] = pc_data;
			    /* channel;_instruments_index++; */
			fprintf(stderr, "\tPRGRM CHNGE channel %d, data %d, instrument %s\n", channel, pc_data, MIDI_PC_INSTRUMENT_NAMES[pc_data]);
		    }
		} else {
		    fprintf(stderr, "UNKNOWN PROGRAM CHANGE: %d\n", pc_data);
		}
		len--;
	    }
		break;
	    case 0xD0: // Channel aftertouch
		fgetc(f);
		len--;
		break;
	    case 0xF0: {// SysEx
		int num_bytes = 0;
		uint32_t length = get_variable_length(f, &num_bytes);
		char *sysex_text = malloc(length + 1);
		fread(sysex_text, 1, length, f);
		sysex_text[length] = '\0';
		/* fseek(f, length, SEEK_CUR); */
		len -= (length + num_bytes);
		fprintf(stderr, "\t\tSYSEX HANDLED, length? %d; message: \"%s\"\n", length, sysex_text);
		free(sysex_text);
	    }
		break;
	    default:
		// Handle running status
		if (status < 0x80) {
		    ungetc(status, f);
		    len++;
		    status = prev_status;
		    goto done_get_status;
		    continue;
		} else {
		    fprintf(stderr, "UNHANDLED status byte %x\n", status & 0xF);
		}
		break;
	    }
	    /* len--; */
	}
	prev_status = status;
    }
    if (len != 0) {
	fprintf(stderr, "Error: an unknown error occurred parsing the MIDI file.\n");
    }
    fprintf(stderr, "TRACK index %d summary:\n", track_index);
    for (int i=0; i<16; i++) {
	fprintf(stderr, "\tchannel %d (%s) ON: %d OFF: %d\n", i, MIDI_PC_INSTRUMENT_NAMES[channel_instruments[i]], num_note_ons[i], num_note_offs[i]);
    }
}

int midi_file_open(const char *filepath, bool automatically_add_tracks)//, MIDIClip **mclips)
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    FILE *f = fopen(filepath, "r");
    if (!f) {
	fprintf(stderr, "Unable to open MIDI file at %s:", filepath);
	perror(NULL);
	return -1;
    }

    click_track = timeline_add_click_track(tl);
   
    
    memset(&file_info, '\0', sizeof(struct midi_file));


    channel_name_index = 0;
    
    /* for (int i=0; i<16; i++) { */
    /* 	v_device[i].record_start = 0; */
    /* 	v_device[i].current_clip = mclips[i]; */
    /* } */
    MIDIChunkType t;
    uint32_t len;
    int track_index = 0;
    t = get_chunk_data(f, &len);
    if (t != MIDI_CHUNK_HDR) {
	fprintf(stderr, "Error: unable to parse MIDI file \"%s\": header chunk is missing or cannot be read\n", filepath);
	return -1;
    }
    get_midi_hdr(f);
    if (!file_info.division_fmt.sample_frames_per_division) {
	uint32_t tempo_us_per_quarter = 500000;
	double us_per_tick = (double)tempo_us_per_quarter / (double)file_info.division_fmt.ticks_per_quarter;
	double frames_per_tic = us_per_tick * session_get_sample_rate() / 1000000.0;
	file_info.division_fmt.sample_frames_per_division = frames_per_tic;
    }
    int num_dst_tracks;
    if (file_info.format == 0) {
	v_device = calloc(16, sizeof(MIDIDevice));
	num_dst_tracks = 16;
    } else if (file_info.format == 1) {
	v_device = calloc(file_info.num_tracks, sizeof(MIDIDevice));
	num_dst_tracks = file_info.num_tracks - 1;
    } else {
	num_dst_tracks = 1;
    }

    int sel_track_i = timeline_selected_track(tl) ? timeline_selected_track(tl)->tl_rank : 0;
    int num_clips = num_dst_tracks;
    /* Prompt user with track behavior */
    if (num_dst_tracks > tl->num_tracks - sel_track_i) {
	const char *option_titles[] = {
	    "Add tracks",
	    "Ignore extra tracks"
	};
	char desc[256];

	if (file_info.format == 1) {
	    snprintf(desc, 256, "The MIDI file contains %d tracks, which will not fit in the current timeline.\nWould you like to automatically add more tracks, or ignore tracks that don't fit?", file_info.num_tracks);
	    if (!automatically_add_tracks) {
		int selection = prompt_user(NULL, desc, 2, option_titles);
		if (selection == 0) automatically_add_tracks = true;
	    }
	    if (automatically_add_tracks) {
		while (tl->num_tracks - sel_track_i < num_dst_tracks) {
		    Track *t = timeline_add_track(tl);
		    t->added_from_midi_filepath = filepath;
		}
	    } else {
		num_clips = tl->num_tracks - sel_track_i;
	    }
	} else if (file_info.format == 0) {
	    /* TODO: determine behavior for multi-channel */
	    if (!automatically_add_tracks) {		
		snprintf(desc, 256, "The MIDI file may contain up to 16 tracks, which will not fit in the current timeline.\nWould you like to automatically add more tracks, or ignore tracks that don't fit?");
		int selection = prompt_user(NULL, desc, 2, option_titles);
		if (selection == 0) automatically_add_tracks = true;
	    }
	    if (automatically_add_tracks) {
		while (tl->num_tracks - sel_track_i < num_dst_tracks)  {
		    Track *t = timeline_add_track(tl);
		    t->added_from_midi_filepath = filepath;
		}
		num_clips = 16;
	    } else {
		num_clips = tl->num_tracks - sel_track_i;
	    }
	}
    }
    Track *dst_tracks[num_clips];
    MIDIClip *mclips[num_clips];
    for (int i=0; i<num_clips; i++) {
	Track *i_track = tl->tracks[sel_track_i + i];
	dst_tracks[i] = i_track;
	MIDIClip *mclip = midi_clip_create(NULL, i_track);
	mclips[i] = mclip;
	v_device[i].record_start = tl->play_pos_sframes;
	v_device[i].current_clip = mclip;	
    }
    
    do {
	t = get_chunk_data(f, &len);
	fprintf(stderr, "Chunk of type %d, len %d\n", t, len);
	if (t == MIDI_CHUNK_HDR) {
	    fprintf(stderr, "Error: unable to parse MIDI file \"%s\": multiple heaader chunks present\n", filepath);
	    free(v_device);
	    fclose(f);
	    return -1;	    
	    /* get_midi_hdr(f); */
	} else if (t == MIDI_CHUNK_TRCK) {
	    get_midi_trck(f, len, track_index, mclips, num_clips, tl->play_pos_sframes);
	    track_index++;
	} else {
	    if (fseek(f, len, SEEK_CUR) == -1) {
		perror("fseek:");
	    }
	}
    } while (t != MIDI_CHUNK_UNKNOWN);
    if (fgetc(f) == EOF) {
	if (feof(f)) {
	    printf("Reached EOF.\n");
	} else {
	    perror("Error reading file");
	}


	for (int i=0; i<num_clips; i++) {
	    
	    /* Get the rest of the notes */
	    midi_device_record_chunk(&v_device[i], 0);
	    
	    MIDIClip *mclip = mclips[i];
	    if (mclip->num_notes == 0) {
		midi_clip_destroy(mclip);
		if (dst_tracks[i]->added_from_midi_filepath == filepath) {
		    track_destroy(dst_tracks[i], true);
		}
	    } else {
		if (file_info.format == 0) {
		    mclip->primary_instrument = channel_instruments[i];
		    mclip->primary_instrument_name = MIDI_PC_INSTRUMENT_NAMES[mclip->primary_instrument];
		}
		if (mclip->midi_track_name) {
		    memcpy(mclip->name, mclip->midi_track_name, strlen(mclip->midi_track_name) + 1);
		} else if (mclip->primary_instrument_name) {
		    memcpy(mclip->name, mclip->primary_instrument_name, strlen(mclip->primary_instrument_name) + 1);
		}

		mclip->len_sframes = mclip->notes[mclip->num_notes - 1].end_rel + 1;
		ClipRef *cr = clipref_create(dst_tracks[i], tl->play_pos_sframes, CLIP_MIDI, mclip);
		clipref_reset(cr, true);
		Track *t = dst_tracks[i];
		if (!t->midi_out) {
		    t->synth = synth_create(t);
		    t->midi_out = t->synth;
		    t->midi_out_type = MIDI_OUT_SYNTH;
		}
	    }
	}
	tl->needs_redraw = true;
    }
    session->proj.active_midi_clip_index += num_clips;
    free(v_device);
    fclose(f);
    return 0;    
}


static void midi_serialize_event(FILE *f, PmEvent event)
{    
    int32_ser_le(f, &event.timestamp);
    uint32_ser_le(f, &event.message);
	  
}

void midi_serialize_events(FILE *f, PmEvent *events, uint32_t num_events)
{
    uint32_ser_le(f, &num_events);
    for (uint32_t i=0; i<num_events; i++) {
	midi_serialize_event(f, events[i]);
    }
}

static void midi_deserialize_event(FILE *f, PmEvent *dst)
{
    dst->timestamp = int32_deser_le(f);
    dst->message = uint32_deser_le(f);
}

uint32_t midi_deserialize_events(FILE *f, PmEvent **events)
{
    uint32_t num_events = uint32_deser_le(f);
    *events = malloc(sizeof(PmEvent) * num_events);
    for (uint32_t i=0; i<num_events; i++) {
	midi_deserialize_event(f, (*events) + i);
    }
    return num_events;
}

