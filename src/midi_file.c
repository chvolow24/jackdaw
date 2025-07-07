#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "midi_clip.h"
#include "midi_io.h"
#include "portmidi.h"

typedef enum {
    MIDI_CHUNK_UNKNOWN,
    MIDI_CHUNK_HDR,
    MIDI_CHUNK_TRCK
} MIDIChunkType;

static MIDIDevice v_device;
static uint32_t running_ts = 0;

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

static uint32_t get_len32(FILE *f)
{
    uint8_t bytes[4];
    fread(bytes, 1, 4, f);
    uint32_t ret = (uint32_t)bytes[0] << 24;
    ret += (uint32_t)bytes[1] << 16;
    ret += (uint32_t)bytes[2] << 8;
    ret += (uint32_t)bytes[3];
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
    *len = get_len32(f);
    return t;
}

/* already read "MThd" and length */
static void get_midi_hdr(FILE *f)
{
    uint16_t format = get_16(f);
    fprintf(stderr, "FORMAT: %d\n", format);
    uint16_t ntracks = get_16(f);
    fprintf(stderr, "TRACKS: %d\n", ntracks);
    uint16_t division = get_16(f);
    fprintf(stderr, "DIVISION: %d\n", division);
    uint8_t division_msb = division >> 15;
    fprintf(stderr, "\tmsb == %d\n", division_msb);
    if (division_msb == 0) {
	uint16_t rest = division & 0x7FFF;
	fprintf(stderr, "\tRest? %d\n", rest);
    }
}

/* already read MTrk and length */
static void get_midi_trck(FILE *f, int32_t len)
{
    while (len > 0) {
	PmEvent e;
	e.timestamp = running_ts;

	/* fprintf(stderr, "len: %d\n", len); */
	int num_bytes;
	uint32_t delta_time = get_variable_length(f, &num_bytes);
	fprintf(stderr, "\tEvent delta : %d, TS: %d\n", delta_time, e.timestamp);
	running_ts += delta_time;
	len -= num_bytes;	
	uint8_t status = fgetc(f);
	len--;
	static uint8_t prev_status;
    done_get_status:
	fprintf(stderr, "\tSTATUS: %x\n", status & 0xF0);
	
	if (status == 0xFF) {
	    uint8_t type = fgetc(f);	    
	    uint32_t length = get_variable_length(f, &num_bytes);
	    fseek(f, length, SEEK_CUR); /* IGNORE META EVENT */
	    len -= num_bytes + length + 1;
	    fprintf(stderr, "Ignore meta event\n");
	} else if ((status & 0xF0) == 0x90) { /* NOTE ON */
	    uint8_t note = fgetc(f);
	    uint8_t velocity = fgetc(f);
	    uint8_t channel = status & 0x0F;
	    fprintf(stderr, "\t\tChannel %d note %d vel %d\n", channel, note, velocity);
	    len -= 3;
	    e.message = Pm_Message(status, note, velocity);
	    v_device.buffer[v_device.num_unconsumed_events] = e;
	    v_device.num_unconsumed_events++;
	    if (v_device.num_unconsumed_events == PM_EVENT_BUF_NUM_EVENTS) {
		midi_device_record_chunk(&v_device);
		v_device.num_unconsumed_events = 0;
		/* fprintf(stderr, "Early return\n"); */
		/* return; */
		/* exit(0); */
	    }
	} else if ((status & 0xF0) == 0x80) { /* NOTE OFF */
	    uint8_t note = fgetc(f);
	    uint8_t velocity = fgetc(f);
	    uint8_t channel = status & 0x0F;
	    fprintf(stderr, "\t\t(off) Channel %d note %d vel %d\n", channel, note, velocity);
	    len -= 3;
	    e.message = Pm_Message(status, note, velocity);
	    v_device.buffer[v_device.num_unconsumed_events] = e;
	    v_device.num_unconsumed_events++;
	    fprintf(stderr, "NUM UNCONSUMED: %d\n", v_device.num_unconsumed_events);
	    if (v_device.num_unconsumed_events == PM_EVENT_BUF_NUM_EVENTS) {
		fprintf(stderr, "RECORD CHUNK!!!!!!\n");
		midi_device_record_chunk(&v_device);
		v_device.num_unconsumed_events = 0;
		/* fprintf(stderr, "Early return\n"); */
		/* return; */
		/* exit(0); */
	    }
	} else {
	    fprintf(stderr, "IN SWITCH x 0xF0: %x\n", status & 0xF0);
	    switch (status & 0xF0) {
	    case 0xA0: // Aftertouch
	    case 0xB0: // Controller
	    case 0xE0: // Pitch bend
		fgetc(f); fgetc(f);
		break;
	    case 0xC0: // Program change
	    case 0xD0: // Channel aftertouch
		fgetc(f);
		break;
	    case 0xF0: {// SysEx
		int num_bytes = 0;
		uint32_t length = get_variable_length(f, &num_bytes);
		fprintf(stderr, "\t\tSYSEX HANDLED, length? %d\n", length);
		fseek(f, length, SEEK_CUR);
		len -= length + num_bytes;
	    }
		break;
	    default:
		// Handle running status
		if (status < 0x80) {
		    fprintf(stderr, "\t...running status\n");
		    fputc(status, f);
		    status = prev_status;
		    goto done_get_status;
		    continue;
		} else {
		    fprintf(stderr, "UNHANDLES status byte %x\n", status & 0xF);
		}
		break;
	    }
	    /* len--; */
	}
	prev_status = status;
    }

    
}

int midi_file_read(const char *filepath, MIDIClip *mclip)
{
    /* if (!mclip) exit(1); */
    running_ts = 0;
    FILE *f = fopen(filepath, "r");
    if (!f) {
	fprintf(stderr, "Unable to open MIDI file at %s:", filepath);
	perror(NULL);
	return -1;
    }
    v_device.record_start = 0;
    /* midi_clip_init(mclip); */
    v_device.current_clip = mclip;
    MIDIChunkType t;
    uint32_t len;
    do {
	t = get_chunk_data(f, &len);
	fprintf(stderr, "Chunk of type %d, len %d\n", t, len);
	if (t == MIDI_CHUNK_HDR) {
	    get_midi_hdr(f);
	} else if (t == MIDI_CHUNK_TRCK) {
	    get_midi_trck(f, len);
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
}
    
    return 0;    
}

/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <stdint.h> */
/* #include <string.h> */
/* #include "note.h" */
/* #include "portmidi.h" */
/* typedef struct midi_event { */
/*     uint32_t delta_time; */
/*     uint8_t event_type; */
/*     uint8_t channel; */
/*     uint8_t data[2]; */
/* } MidiEvent; */

/* typedef struct midi_header { */
/*     uint16_t format; */
/*     uint16_t tracks; */
/*     uint16_t division; */
/* } MidiHeader; */

/* typedef struct note_array { */
/*     Note *notes; */
/*     size_t count; */
/*     size_t capacity; */
/* } NoteArray; */

/* // Default tempo (120 BPM) in microseconds per quarter note */
/* #define DEFAULT_TEMPO 500000 */

/* static uint32_t read_variable_length(FILE *file) { */
/*     uint32_t value = 0; */
/*     uint8_t byte; */

/*     PmEvent t; */
/*     do { */
/*         byte = fgetc(file); */
/*         value = (value << 7) | (byte & 0x7F); */
/*     } while (byte & 0x80); */
    
/*     return value; */
/* } */

/* static uint32_t read_32bit(FILE *file) { */
/*     uint32_t value = 0; */
/*     value |= fgetc(file) << 24; */
/*     value |= fgetc(file) << 16; */
/*     value |= fgetc(file) << 8; */
/*     value |= fgetc(file); */
/*     return value; */
/* } */

/* static uint16_t read_16bit(FILE *file) { */
/*     uint16_t value = 0; */
/*     value |= fgetc(file) << 8; */
/*     value |= fgetc(file); */
/*     return value; */
/* } */

/* static int parse_midi_header(FILE *file, MidiHeader *header) { */
/*     char id[4]; */
/*     if (fread(id, 1, 4, file) != 4 || memcmp(id, "MThd", 4) != 0) { */
/*         return 0; */
/*     } */
    
/*     uint32_t length = read_32bit(file); */
/*     if (length != 6) { */
/*         return 0; */
/*     } */
    
/*     header->format = read_16bit(file); */
/*     header->tracks = read_16bit(file); */
/*     header->division = read_16bit(file); */
    
/*     return 1; */
/* } */

/* static void note_array_init(NoteArray *array) { */
/*     array->notes = NULL; */
/*     array->count = 0; */
/*     array->capacity = 0; */
/* } */

/* static void note_array_add(NoteArray *array, Note note) { */
/*     if (array->count >= array->capacity) { */
/*         array->capacity = array->capacity ? array->capacity * 2 : 16; */
/*         array->notes = realloc(array->notes, array->capacity * sizeof(Note)); */
/*     } */
/*     array->notes[array->count++] = note; */
/* } */

/* static void note_array_free(NoteArray *array) { */
/*     free(array->notes); */
/*     array->notes = NULL; */
/*     array->count = 0; */
/*     array->capacity = 0; */
/* } */

/* static double ticks_to_seconds(uint32_t ticks, uint16_t division, uint32_t tempo) { */
/*     // Tempo is microseconds per quarter note */
/*     return (double)ticks * tempo / (division * 1000000.0); */
/* } */

/* static int32_t seconds_to_samples(double seconds, uint32_t sample_rate) { */
/*     return (int32_t)(seconds * sample_rate + 0.5); */
/* } */

/* NoteArray* parse_midi_file(const char *filename, uint32_t sample_rate) { */
/*     FILE *file = fopen(filename, "rb"); */
/*     if (!file) { */
/*         return NULL; */
/*     } */
    
/*     MidiHeader header; */
/*     if (!parse_midi_header(file, &header)) { */
/*         fclose(file); */
/*         return NULL; */
/*     } */
    
/*     NoteArray *note_arrays = malloc(header.tracks * sizeof(NoteArray)); */
/*     if (!note_arrays) { */
/*         fclose(file); */
/*         return NULL; */
/*     } */
    
/*     for (int i = 0; i < header.tracks; i++) { */
/*         note_array_init(&note_arrays[i]); */
/*     } */
    
/*     // Process each track */
/*     for (int track = 0; track < header.tracks; track++) { */
/*         char track_id[4]; */
/*         if (fread(track_id, 1, 4, file) != 4 || memcmp(track_id, "MTrk", 4) != 0) { */
/*             continue; */
/*         } */
        
/*         uint32_t track_length = read_32bit(file); */
/*         uint32_t track_end = ftell(file) + track_length; */
        
/*         uint32_t current_time = 0; */
/*         uint32_t tempo = DEFAULT_TEMPO; */
/*         Note **active_notes = malloc(128 * sizeof(Note*)); */
/*         memset(active_notes, 0, 128 * sizeof(Note*)); */
        
/*         while (ftell(file) < track_end) { */
/*             uint32_t delta_time = read_variable_length(file); */
/*             current_time += delta_time; */
            
/*             uint8_t status = fgetc(file); */
/*             if (status == 0xFF) { */
/*                 // Meta event */
/*                 uint8_t type = fgetc(file); */
/*                 uint32_t length = read_variable_length(file); */
                
/*                 if (type == 0x51) { // Tempo change */
/*                     if (length == 3) { */
/*                         tempo = (fgetc(file) << 16) | (fgetc(file) << 8) | fgetc(file); */
/*                     } else { */
/*                         fseek(file, length, SEEK_CUR); */
/*                     } */
/*                 } else { */
/*                     fseek(file, length, SEEK_CUR); */
/*                 } */
/*                 continue; */
/*             } */
            
/*             if ((status & 0xF0) == 0x90) { // Note on */
/*                 uint8_t note = fgetc(file); */
/*                 uint8_t velocity = fgetc(file); */
                
/*                 if (velocity > 0) { */
/*                     if (active_notes[note]) { */
/*                         // Handle overlapping notes - end previous note */
/*                         Note *n = active_notes[note]; */
/*                         n->end_rel = seconds_to_samples( */
/*                             ticks_to_seconds(current_time, header.division, tempo), */
/*                             sample_rate); */
/*                     } */
                    
/*                     Note new_note = { */
/*                         .note = note, */
/*                         .velocity = velocity, */
/*                         .start_rel = seconds_to_samples( */
/*                             ticks_to_seconds(current_time, header.division, tempo), */
/*                             sample_rate), */
/*                         .end_rel = 0 // Will be set on note off */
/*                     }; */
/*                     note_array_add(&note_arrays[track], new_note); */
/*                     active_notes[note] = &note_arrays[track].notes[note_arrays[track].count - 1]; */
/*                 } else { */
/*                     // Note on with velocity 0 is treated as note off */
/*                     if (active_notes[note]) { */
/*                         Note *n = active_notes[note]; */
/*                         n->end_rel = seconds_to_samples( */
/*                             ticks_to_seconds(current_time, header.division, tempo), */
/*                             sample_rate); */
/*                         active_notes[note] = NULL; */
/*                     } */
/*                 } */
/*             } else if ((status & 0xF0) == 0x80) { // Note off */
/*                 uint8_t note = fgetc(file); */
/*                 fgetc(file); // velocity (ignored for note off) */
                
/*                 if (active_notes[note]) { */
/*                     Note *n = active_notes[note]; */
/*                     n->end_rel = seconds_to_samples( */
/*                         ticks_to_seconds(current_time, header.division, tempo), */
/*                         sample_rate); */
/*                     active_notes[note] = NULL; */
/*                 } */
/*             } else { */
/*                 // Skip other events */
/*                 switch (status & 0xF0) { */
/*                     case 0xA0: // Aftertouch */
/*                     case 0xB0: // Controller */
/*                     case 0xE0: // Pitch bend */
/*                         fgetc(file); fgetc(file); */
/*                         break; */
/*                     case 0xC0: // Program change */
/*                     case 0xD0: // Channel aftertouch */
/*                         fgetc(file); */
/*                         break; */
/*                     default: */
/*                         // Handle running status */
/*                         if (status < 0x80) { */
/*                             // This is a data byte for previous event */
/*                             // Implementation depends on previous event */
/*                             // For simplicity, we'll skip it */
/*                             continue; */
/*                         } */
/*                         break; */
/*                 } */
/*             } */
/*         } */
        
/*         free(active_notes); */
/*     } */
    
/*     fclose(file); */
/*     return note_arrays; */
/* } */

/* void free_midi_notes(NoteArray *notes, int track_count) { */
/*     for (int i = 0; i < track_count; i++) { */
/*         note_array_free(&notes[i]); */
/*     } */
/*     free(notes); */
/* } */

/* // Example usage: */
/* /\* */
/* int main() { */
/*     uint32_t sample_rate = 44100; // Example sample rate */
/*     NoteArray *notes = parse_midi_file("example.mid", sample_rate); */
    
/*     if (notes) { */
/*         for (int track = 0; track < 1; track++) { // Just show first track for example */
/*             printf("Track %d has %zu notes:\n", track, notes[track].count); */
/*             for (size_t i = 0; i < notes[track].count; i++) { */
/*                 Note n = notes[track].notes[i]; */
/*                 printf("Note %d: vel=%d, start=%d, end=%d\n",  */
/*                        n.note, n.velocity, n.start_rel, n.end_rel); */
/*             } */
/*         } */
/*         free_midi_notes(notes, 1); // Assuming we know there's 1 track */
/*     } */
    
/*     return 0; */
/* } */
/* *\/ */
