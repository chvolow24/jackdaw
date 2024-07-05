/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    audio_connection.c

    * Query for available audio connections
    * Open and close individual audio connections
 *****************************************************************************************************************/


#include <pthread.h>
#include "audio_connection.h"
#include "project.h"
#include "pure_data.h"


/* #define DEVICE_BUFLEN_SECONDS 2 /\* TODO: reduce, and write to clip during recording *\/ */
#define DEVICE_BUFLEN_CHUNKS 1024

#define PD_BUFLEN_CHUNKS 1024

extern Project *proj;



int query_audio_connections(Project *proj, int iscapture)
{
    AudioConn **conn_list;
    if (iscapture) {
	conn_list = proj->record_conns;
    } else {
	conn_list = proj->playback_conns;
    }

    
    AudioConn *default_conn = calloc(sizeof(AudioConn), 1);
    AudioDevice *default_dev = &default_conn->c.device;
    char *name;
    if (SDL_GetDefaultAudioInfo(&name, &(default_dev->spec), iscapture) != 0) {
        default_conn->iscapture = iscapture;
        /* default_conn->name = "(no device found)"; */
	strcpy(default_conn->name, "(no device found)");
        fprintf(stderr, "Error: unable to retrieve default %s device info. %s\n", iscapture ? "input" : "output", SDL_GetError());
    }
    strncpy(default_conn->name, name, MAX_CONN_NAMELENGTH);
    default_conn->name[MAX_CONN_NAMELENGTH - 1] = '\0';
    fprintf(stdout, "FOUND default device, name %s\n", default_conn->name);
    /* exit(0); */
    default_conn->open = false;
    default_conn->active = false;
    default_conn->available = true;
    default_conn->index = 0;
    default_conn->iscapture = iscapture;
    default_dev->write_bufpos_samples = 0;
    default_dev->rec_buffer = NULL;
    default_dev->id = 0;
    // memset(default_dev->rec_buffer, '\0', BUFFLEN / 2);
    /* fprintf(stdout, "Default device %s at index %d\n", default_conn->name, default_conn->index); */

    int num_devices = SDL_GetNumAudioDevices(iscapture);
    conn_list[0] = default_conn;
    for (int i=0,j=1; i<num_devices; i++,j++) {
	AudioConn *conn = calloc(sizeof(AudioConn), 1);
        AudioDevice *dev = &conn->c.device;
	const char *name = SDL_GetAudioDeviceName(i, iscapture);
        strncpy(conn->name, name, MAX_CONN_NAMELENGTH);
	conn->name[MAX_CONN_NAMELENGTH - 1] = '\0';
        if (strcmp(conn->name, default_conn->name) == 0) {
	    /* default_conn->index = j; */
            free(conn);
            j--;
            continue;
        }
        /* conn->open = false; */
        /* conn->active = false; */
	conn->available = true;
        conn->index = j;
        conn->iscapture = iscapture;
        /* dev->write_bufpos_samples = 0; */
        // memset(dev->rec_buffer, '\0', BUFFLEN / 2);
        SDL_AudioSpec spec;
        if (SDL_GetAudioDeviceSpec(i, iscapture, &spec) != 0) {
            fprintf(stderr, "Error getting device spec: %s\n", SDL_GetError());
        };
        dev->spec = spec;
        dev->rec_buffer = NULL;
	dev->id = 0;
        conn_list[j] = conn;
	/* fprintf(stdout, "Connection %s at index %d\n", conn->name, conn->index); */
        /* fprintf(stderr, "\tFound device: %s, index: %d\n", dev->name, dev->index); */

    }

    int num_conns = num_devices;
    /* Check for pure data */
    if (iscapture) {
	AudioConn *pd = calloc(sizeof(AudioConn), 1);
	pd->type = PURE_DATA;
	strcpy(pd->name, "Pure data");
	/* pd->name = "pure data"; */
	pd->index = num_devices;
	pd->iscapture = iscapture;
	pd->available = true;
	/* pd->c.pd.buf_lock = SDL_CreateMutex(); */
	conn_list[num_conns] = pd;
	num_conns++;

	AudioConn *jdaw = calloc(sizeof(AudioConn), 1);
	jdaw->type = JACKDAW;
	strcpy(jdaw->name, "Jackdaw out");
	jdaw->index = num_devices;
	jdaw->iscapture = iscapture;
	jdaw->available = true;
	conn_list[num_conns] = jdaw;
	num_conns++;
    }
    if (!iscapture) {
	fprintf(stdout, "PLAYBACK CONNS: %d\n", num_conns);
    }
    return num_conns;

}


void transport_playback_callback(void *user_data, uint8_t *stream, int len);
void transport_record_callback(void *user_data, uint8_t *stream, int len);

int audioconn_open(Project *proj, AudioConn *conn)
{
    switch (conn->type) {
    case DEVICE: {
	AudioDevice *device = &conn->c.device;
	SDL_AudioSpec obtained;
	SDL_zero(obtained);
	SDL_zero(device->spec);

	/* Project determines high-level audio settings */
	device->spec.format = AUDIO_S16LSB;
	device->spec.samples = proj->chunk_size_sframes;
	device->spec.freq = proj->sample_rate;

	device->spec.channels = proj->channels;
	device->spec.callback = conn->iscapture ? transport_record_callback : transport_playback_callback;
	device->spec.userdata = device;

	/* for (int i=0; i<10; i++) { */
	if ((device->id = SDL_OpenAudioDevice(conn->name, conn->iscapture, &(device->spec), &(obtained), 0)) > 0) {
	    fprintf(stdout, "ID: %d\n", device->id);
	    device->spec = obtained;
	    conn->open = true;
	    fprintf(stderr, "Successfully opened device %s, with id: %d\n", conn->name, device->id);
	} else {
	    conn->open = false;
	    fprintf(stderr, "Error opening audio device %s : %s\n", conn->name, SDL_GetError());
	    return 1;
	/* } */
	}
	/* exit(1); */

	if (conn->iscapture) {
	    /* fprintf(stdout, "Dev %s\n:ch %d, freq %d, format %d (== %d)\n", device->name, obtained.channels, obtained.freq, obtained.format, AUDIO_S16LSB); */
	    /* exit(1); */
	    /* device->rec_buf_len_samples = proj->sample_rate * DEVICE_BUFLEN_SECONDS * device->spec.channels; */
	    device->rec_buf_len_samples = DEVICE_BUFLEN_CHUNKS * proj->chunk_size_sframes * proj->channels;
	    uint32_t device_buf_len_bytes = device->rec_buf_len_samples * sizeof(int16_t);
	    if (!device->rec_buffer) {
		device->rec_buffer = malloc(device_buf_len_bytes);
	    }
	    device->write_bufpos_samples = 0;
	    if (!(device->rec_buffer)) {
		fprintf(stderr, "Error: unable to allocate space for device buffer.\n");
	    }
	}
    }
	break;
    case PURE_DATA:
	fprintf(stdout, "Opening pd\n");
	PdConn *pdconn = &conn->c.pd;
	if (conn->iscapture) {
	    if (!pdconn->rec_buffer_L) {
		pdconn->rec_buf_len_sframes = PD_BUFLEN_CHUNKS * proj->chunk_size_sframes;
		if (!pdconn->rec_buffer_L) pdconn->rec_buffer_L = malloc(sizeof(float) * pdconn->rec_buf_len_sframes);
		if (!pdconn->rec_buffer_R) pdconn->rec_buffer_R = malloc(sizeof(float) * pdconn->rec_buf_len_sframes);
		fprintf(stdout, "allocated %d samples in buf\n", pdconn->rec_buf_len_sframes);
		pdconn->write_bufpos_sframes = 0;
		if (pdconn->rec_buffer_L == NULL || pdconn->rec_buffer_R == NULL) {
		    fprintf(stderr, "Error: unable to allocate space for pd buffers\n");
		}
	    }
	}
	/* SDL_UnlockMutex(pdconn->buf_lock); */
	fprintf(stdout, "Successfully opened pd conn\n");
	/* conn->index = -1; */
	conn->open = true;
	break;
    case JACKDAW:
	if (conn->iscapture) {
	    conn->c.jdaw.rec_buf_len_sframes = PD_BUFLEN_CHUNKS * proj->chunk_size_sframes;
	    if (!conn->c.jdaw.rec_buffer_L) conn->c.jdaw.rec_buffer_L = malloc(sizeof(float) * conn->c.jdaw.rec_buf_len_sframes);
	    if (!conn->c.jdaw.rec_buffer_R) conn->c.jdaw.rec_buffer_R = malloc(sizeof(float) * conn->c.jdaw.rec_buf_len_sframes);
	}
	break;
    }
    return 0;
}

/* int audioconn_open(Project *proj, AudioConn *conn) { */
/*     int ret = 0; */
/*     switch (conn->type) { */
/*     case DEVICE: */
/* 	ret = device_open(proj, &conn->c.device); */
/* 	break; */
/*     case PURE_DATA: */
/* 	break; */
/*     } */
/*     return ret; */

/* } */

/* static void device_destroy(AudioDevice *device) */
/* { */
/*     if (device->rec_buffer) { */
/*         free(device->rec_buffer); */
/*         free(device); */
/*     } */
/* } */


void audioconn_destroy(AudioConn *conn)
{
    /* audioconn_close(conn); */
    /* fprintf(stdout, "Destroying %s\n", conn->name); */
    float *buf;
    int16_t *intbuf;
    switch (conn->type) {
    case PURE_DATA:
	if ((buf = conn->c.pd.rec_buffer_L))
	    free(buf);
	if ((buf = conn->c.pd.rec_buffer_R))
	    free(buf);
	break;
    case DEVICE:
	if ((intbuf= conn->c.device.rec_buffer))
	    free(intbuf);
	break;
    case JACKDAW:
	if ((buf = conn->c.jdaw.rec_buffer_L))
	    free(buf);
	if ((buf = conn->c.jdaw.rec_buffer_R))
	    free(buf);
	break;
    }
    free(conn);
}

static void device_close(AudioDevice *device)
{
    if (device->rec_buffer) {
	free(device->rec_buffer);
	device->rec_buffer = NULL;
    }
    /* fprintf(stdout, "CLOSING device %s, id: %d\n",device->name, device->id); */
    SDL_CloseAudioDevice(device->id);
    device->id = 0;
}

void audioconn_close(AudioConn *conn)
{
    float **buf;
    conn->open = false;
    switch (conn->type) {
    case DEVICE:
	device_close(&conn->c.device);
	break;
    case PURE_DATA:
	/* SDL_TryLockMutex(conn->c.pd.buf_lock); */
	/* if ((buf = &conn->c.pd.rec_buffer_L)) { */
	/*     free(*buf); */
	/*     *buf = NULL; */
	/* } */
	/* if ((buf = &conn->c.pd.rec_buffer_R)) { */
	/*     free(*buf); */
	/*     *buf = NULL; */
	/* } */
	break;
    case JACKDAW:
	if ((buf = &conn->c.pd.rec_buffer_L)) {
	    free(*buf);
	    *buf = NULL;
	}
	if ((buf = &conn->c.pd.rec_buffer_R)) {
	    free(*buf);
	    *buf = NULL;
	}
	break;
    }
    
}

static void device_start_playback(AudioDevice *dev)
{
    SDL_PauseAudioDevice(dev->id, 0);
}


void audioconn_start_playback(AudioConn *conn)
{
    switch (conn->type) {
    case DEVICE:
	device_start_playback(&conn->c.device);
	break;
    case PURE_DATA:
	break;
    case JACKDAW:
	break;
    }
}


static void device_stop_playback(AudioDevice *dev)
{
    SDL_PauseAudioDevice(dev->id, 1);
}

void audioconn_stop_playback(AudioConn *conn)
{
    switch (conn->type) {
    case DEVICE:
	device_stop_playback(&conn->c.device);
	break;
    case PURE_DATA:
	break;
    case JACKDAW:
	break;
    }
}

static void device_stop_recording(AudioDevice *dev)
{
    SDL_PauseAudioDevice(dev->id, 1);
    /* dev->write_bufpos_samples = 0; */
    /* device_close(dev); */
}

void audioconn_stop_recording(AudioConn *conn)
{
    switch (conn->type) {
    case DEVICE:
	device_stop_recording(&conn->c.device);
	break;
    case PURE_DATA:
	break;
    case JACKDAW:
	break;
    }
}



static void device_start_recording(AudioDevice *dev)
{
    /* if (!dev->open) { */
    /* 	device_open(proj, dev); */
    /* } */
    SDL_PauseAudioDevice(dev->id, 0);
}

void audioconn_start_recording(AudioConn *conn)
{
    switch (conn->type) {
    case DEVICE:
	device_start_recording(&conn->c.device);
	break;
    case PURE_DATA: {
	pthread_t pd_thread;
	pthread_create(&pd_thread, NULL, pd_jackdaw_record_on_thread, conn);
    }
	break;
    case JACKDAW:
	break;
    }
}

void audioconn_handle_connection_event(int index_or_id, int iscapture, int event_type)
{
    fprintf(stdout, "\n\nEvent: %s device %s with id %d\n", iscapture ? "recording" : "playback", event_type == SDL_AUDIODEVICEREMOVED ? "removed" : "added", index_or_id);
    fprintf(stdout, "\n\tCurrent project playback devices:\n");
    for (uint8_t i=0; i<proj->num_playback_conns; i++) {
	fprintf(stdout, "\t\tID %d: %s\n", proj->playback_conns[i]->c.device.id, proj->playback_conns[i]->name);
    }
    fprintf(stdout, "\n\tCurrent project record devices:\n");
    for (uint8_t i=0; i<proj->num_record_conns; i++) {
	fprintf(stdout, "\t\tID %d: %s\n", proj->record_conns[i]->c.device.id, proj->record_conns[i]->name);
    }

    /* fprintf(stdout,src/ "current playback device id: %d\n", proj->playback_conn->c.device.id); */
    AudioConn **arr = iscapture ? proj->record_conns : proj->playback_conns;
    uint8_t *num_previous = iscapture ? &proj->num_record_conns : &proj->num_playback_conns;
    if (event_type == SDL_AUDIODEVICEREMOVED) {
	AudioConn *removed_conn = NULL;
	uint8_t remove_at = 0;
	int num_new = SDL_GetNumAudioDevices(iscapture);
	/* const char **names = malloc(sizeof(char *) * num_new); */
	/* for (uint8_t i=0; i<num_new; i++) { */
	/*     names[i] = SDL_GetAudioDeviceName(i, iscapture); */
	/* } */

	if (index_or_id) {
	    SDL_PauseAudioDevice(index_or_id, 1);
	    /* SDL_CloseAudioDevice(index_or_id); */
	    for (int i=0; i<*num_previous; i++) {
		if (arr[i]->c.device.id == index_or_id) {
		    removed_conn = arr[i];
		    remove_at = i;
		    fprintf(stdout, "Found id %d w/ name %s\n", arr[i]->c.device.id, arr[i]->name);
		    break;
		}
		/* SDL_PauseAudioDevice( */
	    }
	} else {
	    for (int i=0; i<*num_previous; i++) {
		AudioConn *check = arr[i];
		if (check->type != DEVICE) continue;
		bool found = false;
		for (int j=0; j<num_new; j++) {
		    if (strcmp(SDL_GetAudioDeviceName(j, iscapture), check->name) == 0) {
			found = true;
			break;
		    }
		}
		if (!found) {
		    removed_conn = check;
		    remove_at = i;
		    break;
		}
	    }
	}
	
	/* free(names);	 */
	if (!removed_conn) {
	    fprintf(stderr, "Fatal error: removed audio connection could not be found\n");
	    exit(1);
	}
	/* audioconn_close(removed_conn); */
	fprintf(stdout, "%s device %s removed\n", iscapture ? "Recording" : "Playback", removed_conn->name);
	for (uint8_t i=remove_at + 1; i<*num_previous; i++) {
	    arr[i - 1] = arr[i];
	}
	(*num_previous)--;
	if (*num_previous == 0) {
	    fprintf(stderr, "Warning: no audio devices available\n");
	} else {
	    /* AudioConn *replacement = NULL; */
	    /* char *new_default_name; */
	    /* SDL_AudioSpec spec; /\* not actually used *\/ */
	    /* SDL_GetDefaultAudioInfo(&new_default_name, &spec, iscapture); */
	    /* fprintf(stdout, "Replacement name! %s\n", new_default_name); */
	    /* for (uint8_t i=0; i<*num_previous; i++) { */
	    /* 	if (strcmp(arr[i]->name, new_default_name) == 0) { */
	    /* 	    replacement = arr[i]; */
	    /* 	} */
	    /* } */
	    
	    /* if (!replacement) replacement = arr[0]; */
	    AudioConn *replacement = arr[0];
	    if (iscapture) {
		for (uint8_t i=0; i<proj->num_timelines; i++) {
		    Timeline *tl = proj->timelines[i];
		    for (uint8_t j=0; j<tl->num_tracks; j++) {
			Track *track = tl->tracks[j];
			if (track->input == removed_conn) {
			    track->input = replacement;
			    textbox_set_value_handle(track->tb_input_name, replacement->name);
			}
		    }
		}
	    } else {
		if (proj->playback_conn == removed_conn) {
		    proj->playback_conn = replacement;
		    textbox_set_value_handle(proj->tb_out_value, replacement->name);
		    if (audioconn_open(proj, proj->playback_conn) != 0) {
			fprintf(stderr, "Error: failed to open default audio conn \"%s\". More info: %s\n", proj->playback_conn->name, SDL_GetError());
		    }
		}
	    }
	}
	audioconn_destroy(removed_conn);	
    } else {
	AudioConn *new = calloc(1, sizeof(AudioConn));
	new->type = DEVICE;
	strncpy(new->name, SDL_GetAudioDeviceName(index_or_id, iscapture), MAX_CONN_NAMELENGTH);
	fprintf(stdout, "ADDED %s device %s\n", iscapture ? "recording" : "playback", new->name);
	new->name[MAX_CONN_NAMELENGTH-1] = '\0';
	new->available = true;
        new->index = index_or_id;
        new->iscapture = iscapture;
        /* dev->write_bufpos_samples = 0; */
        // memset(dev->rec_buffer, '\0', BUFFLEN / 2);
        SDL_AudioSpec spec;
        if (SDL_GetAudioDeviceSpec(index_or_id, iscapture, &spec) != 0) {
            fprintf(stderr, "Error getting device spec: %s\n", SDL_GetError());
        };
        new->c.device.spec = spec;
        /* new->c.device.rec_buffer = NULL; */
        arr[*num_previous] = new;
	(*num_previous)++;
	/* fprintf(stdout, "Connection %s at index %d\n", conn->name, conn->index); */

	
    }
    fprintf(stdout, "\t\t\tEXIT handle device event\n");
}

void audioconn_reset_chunk_size(AudioConn *c, uint16_t new_chunk_size)
{
    if (c->open) {
	audioconn_close(c);
    }
    switch (c->type) {
    case DEVICE:
	c->c.device.spec.size = new_chunk_size;
	break;
    default:
	break;
    }

}
void ___audioconn_handle_connection_event(int id, int iscapture, int event_type)
{
    /* if (!proj) { */
    /* 	return; */
    /* } */
    /* const char *eventstr = event_type == SDL_AUDIODEVICEREMOVED ? "closed" : "added"; */
    /* AudioConn **arr = iscapture ? proj->record_conns : proj->playback_conns; */
    /* int num_conns = iscapture ? proj->num_record_conns - 1 : proj->num_playback_conns; */
    /* AudioConn *conn; */
    /* if (event_type == SDL_AUDIODEVICEREMOVED) { */
    /* 	for (uint8_t i=0; i<num_conns; i++) { */
    /* 	    conn = arr[i]; */
    /* 	    if (conn->c.device.id == id) break; */
    /* 	    else conn = NULL; */
    /* 	} */
    /* 	if (conn) { */
    /* 	    conn->available = false; */
    /* 	    fprintf(stdout, "%d Audio Connection to %s (id %d) has been %s\n", iscapture, conn->name, id, eventstr); */
    /* 	    AudioConn *replacement = NULL; */
    /* 	    for (uint8_t i=0; i<num_conns; i++) { */
    /* 		if (arr[i]->available) { */
    /* 		    replacement = arr[i]; */
    /* 		    break; */
    /* 		} */
    /* 	    } */
    /* 	    fprintf(stdout, "Replacement? %s\n", replacement->name); */
    /* 	    if (replacement) { */
    /* 		if (conn == proj->playback_conn) { */
    /* 		    proj->playback_conn = replacement; */
    /* 		    if (audioconn_open(proj, proj->playback_conn) != 0) { */
    /* 			fprintf(stderr, "Error: failed to open default audio conn \"%s\". More info: %s\n", proj->playback_conn->name, SDL_GetError()); */
    /* 		    } */
    /* 		    textbox_set_value_handle(proj->tb_out_value, proj->playback_conn->name); */
    /* 		} */
    /* 		for (uint8_t i=0; i<proj->num_timelines; i++) { */
    /* 		    Timeline *tl = proj->timelines[i]; */
    /* 		    for (uint8_t j=0; j<tl->num_tracks; j++) { */
    /* 			Track *track = tl->tracks[j]; */
    /* 			if (conn == track->input) { */
    /* 			    track->input = replacement; */
    /* 			    textbox_set_value_handle(track->tb_input_name, track->input->name); */
    /* 			} */
    /* 		    } */
    /* 		    timeline_reset_full(tl); */
    /* 		} */
    /* 	    } */
    /* 	} else { */
    /* 	    fprintf(stderr, "%d ERROR: audio device id %d can't be %s: out of range\n", iscapture, id, eventstr); */
    /* 	    return; */
    /* 	} */
    /* } else { */
    /* 	/\* char *name = SDL_GetAudioDeviceName(id, iscapture); /\\* In the case that a device is added, 'id' refers to index, not SDL_AudioDeviceID *\\/ *\/ */
    /* 	AudioConn *new = audio_conn_create_device(id, iscapture); */
    /* 	new->name = SDL_GetAudioDeviceName(new->index, iscapture); */
    /* 	fprintf(stdout, "Audio device %s added\n", new->name); */
    /* 	if (iscapture) { */
    /* 	    proj->record_conns[proj->num_record_conns] = new; */
    /* 	    proj->num_record_conns++; */
    /* 	} */
	
    /* } */
}
