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
    audio_device.c

    * Query for available audio devices
    * Open and close individual audio devices
 *****************************************************************************************************************/


#include "audio_connection.h"
#include "project.h"


/* #define DEVICE_BUFLEN_SECONDS 2 /\* TODO: reduce, and write to clip during recording *\/ */
#define DEVICE_BUFLEN_CHUNKS 1024

extern Project *proj;

/* static int query_audio_devices(Project *proj, int iscapture) */
/* { */
    /* AudioDevice **device_list; */
    /* if (iscapture) { */
    /* 	device_list = proj->record_devices; */
    /* } else { */
    /* 	device_list = proj->playback_devices; */
    /* } */
    
    /* AudioDevice *default_dev = malloc(sizeof(AudioDevice)); */

    /* if (SDL_GetDefaultAudioInfo((char**) &((default_dev->name)), &(default_dev->spec), iscapture) != 0) { */
    /*     default_dev->iscapture = iscapture; */
    /*     default_dev->name = "(no device found)"; */
    /*     fprintf(stderr, "Error: unable to retrieve default %s device info. %s\n", iscapture ? "input" : "output", SDL_GetError()); */
    /* } */
    /* default_dev->open = false; */
    /* default_dev->active = false; */
    /* default_dev->index = 0; */
    /* default_dev->iscapture = iscapture; */
    /* default_dev->write_bufpos_samples = 0; */
    /* default_dev->rec_buffer = NULL; */
    /* // memset(default_dev->rec_buffer, '\0', BUFFLEN / 2); */


    /* int num_devices = SDL_GetNumAudioDevices(iscapture); */

    /* device_list[0] = default_dev; */
    /* for (int i=0,j=1; i<num_devices; i++,j++) { */
    /*     AudioDevice *dev = malloc(sizeof(AudioDevice)); */
    /*     dev->name = SDL_GetAudioDeviceName(i, iscapture); */
    /*     if (strcmp(dev->name, default_dev->name) == 0) { */
    /*         free(dev); */
    /*         j--; */
    /*         continue; */
    /*     } */
    /*     dev->open = false; */
    /*     dev->active = false; */
    /*     dev->index = j; */
    /*     dev->iscapture = iscapture; */
    /*     dev->write_bufpos_samples = 0; */
    /*     // memset(dev->rec_buffer, '\0', BUFFLEN / 2); */
    /*     SDL_AudioSpec spec; */
    /*     if (SDL_GetAudioDeviceSpec(i, iscapture, &spec) != 0) { */
    /*         fprintf(stderr, "Error getting device spec: %s\n", SDL_GetError()); */
    /*     }; */
    /*     dev->spec = spec; */
    /*     dev->rec_buffer = NULL; */
    /*     device_list[j] = dev; */
    /*     /\* fprintf(stderr, "\tFound device: %s, index: %d\n", dev->name, dev->index); *\/ */

    /* } */
    /* return num_devices; */
/* } */

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
    if (SDL_GetDefaultAudioInfo((char**) &((default_conn->name)), &(default_dev->spec), iscapture) != 0) {
        default_conn->iscapture = iscapture;
        default_conn->name = "(no device found)";
        fprintf(stderr, "Error: unable to retrieve default %s device info. %s\n", iscapture ? "input" : "output", SDL_GetError());
    }
    default_conn->open = false;
    default_conn->active = false;
    default_conn->index = 0;
    default_conn->iscapture = iscapture;
    default_dev->write_bufpos_samples = 0;
    default_dev->rec_buffer = NULL;
    // memset(default_dev->rec_buffer, '\0', BUFFLEN / 2);


    int num_devices = SDL_GetNumAudioDevices(iscapture);
    conn_list[0] = default_conn;
    
    for (int i=0,j=1; i<num_devices; i++,j++) {
	AudioConn *conn = calloc(sizeof(AudioConn), 1);
        AudioDevice *dev = &conn->c.device;
        conn->name = SDL_GetAudioDeviceName(i, iscapture);
        if (strcmp(conn->name, default_conn->name) == 0) {
            free(conn);
            j--;
            continue;
        }
        conn->open = false;
        conn->active = false;
        conn->index = j;
        conn->iscapture = iscapture;
        dev->write_bufpos_samples = 0;
        // memset(dev->rec_buffer, '\0', BUFFLEN / 2);
        SDL_AudioSpec spec;
        if (SDL_GetAudioDeviceSpec(i, iscapture, &spec) != 0) {
            fprintf(stderr, "Error getting device spec: %s\n", SDL_GetError());
        };
        dev->spec = spec;
        dev->rec_buffer = NULL;
        conn_list[j] = conn;
        /* fprintf(stderr, "\tFound device: %s, index: %d\n", dev->name, dev->index); */

    }

    int num_conns = num_devices;
    /* Check for pure data */
    if (iscapture) {
	AudioConn *pd = calloc(sizeof(AudioConn), 1);
	pd->type = PURE_DATA;
	pd->name = "pure data";
	pd->index = num_devices;
	pd->iscapture = iscapture;
	conn_list[num_conns] = pd;
	num_conns++;
    }
    
    return num_conns;

}


void transport_playback_callback(void *user_data, uint8_t *stream, int len);
void transport_record_callback(void *user_data, uint8_t *stream, int len);

/* /\* Returns 0 if device opened successfully, 1 on error *\/ */
/* static int device_open(Project *proj, AudioDevice *device) */
/* { */
/*     SDL_AudioSpec obtained; */
/*     SDL_zero(obtained); */
/*     SDL_zero(device->spec); */

/*     /\* Project determines high-level audio settings *\/ */
/*     device->spec.format = AUDIO_S16LSB; */
/*     device->spec.samples = proj->chunk_size_sframes; */
/*     device->spec.freq = proj->sample_rate; */

/*     device->spec.channels = proj->channels; */
/*     device->spec.callback = device->iscapture ? transport_record_callback : transport_playback_callback; */
/*     device->spec.userdata = device; */

/*     if ((device->id = SDL_OpenAudioDevice(device->name, device->iscapture, &(device->spec), &(obtained), 0)) > 0) { */
/*         device->spec = obtained; */
/*         device->open = true; */
/*         fprintf(stderr, "Successfully opened device %s, with id: %d\n", device->name, device->id); */
/*     } else { */
/*         device->open = false; */
/*         fprintf(stderr, "Error opening audio device %s : %s\n", device->name, SDL_GetError()); */
/*         return 1; */
/*     } */

/*     if (device->iscapture) { */
/* 	/\* fprintf(stdout, "Dev %s\n:ch %d, freq %d, format %d (== %d)\n", device->name, obtained.channels, obtained.freq, obtained.format, AUDIO_S16LSB); *\/ */
/* 	/\* exit(1); *\/ */
/* 	/\* device->rec_buf_len_samples = proj->sample_rate * DEVICE_BUFLEN_SECONDS * device->spec.channels; *\/ */
/* 	device->rec_buf_len_samples = DEVICE_BUFLEN_CHUNKS * proj->chunk_size_sframes * proj->channels; */
/* 	uint32_t device_buf_len_bytes = device->rec_buf_len_samples * sizeof(int16_t); */
/* 	if (!device->rec_buffer) { */
/* 	    device->rec_buffer = malloc(device_buf_len_bytes); */
/* 	} */
/* 	device->write_bufpos_samples = 0; */
/* 	if (!(device->rec_buffer)) { */
/* 	    fprintf(stderr, "Error: unable to allocate space for device buffer.\n"); */
/* 	} */
/*     } */
/*     return 0; */
/* } */

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

	if ((device->id = SDL_OpenAudioDevice(conn->name, conn->iscapture, &(device->spec), &(obtained), 0)) > 0) {
	    device->spec = obtained;
	    conn->open = true;
	    fprintf(stderr, "Successfully opened device %s, with id: %d\n", conn->name, device->id);
	} else {
	    conn->open = false;
	    fprintf(stderr, "Error opening audio device %s : %s\n", conn->name, SDL_GetError());
	    return 1;
	}

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
    case PURE_DATA:
	fprintf(stdout, "TODO: pure data\n");
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

static void device_destroy(AudioDevice *device)
{
    if (device->rec_buffer) {
        free(device->rec_buffer);
        free(device);
    }
}



static void device_close(AudioDevice *device)
{
    /* fprintf(stdout, "CLOSING device %s, id: %d\n",device->name, device->id); */
    SDL_CloseAudioDevice(device->id);
}

void audioconn_close(AudioConn *conn)
{
    conn->open = false;
    switch (conn->type) {
    case DEVICE:
	device_close(&conn->c.device);
	break;
    case PURE_DATA:
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
    }
}

static void device_stop_recording(AudioDevice *dev)
{
    SDL_PauseAudioDevice(dev->id, 1);
    /* dev->write_bufpos_samples = 0; */
    device_close(dev);
}

void audioconn_stop_recording(AudioConn *conn)
{
    switch (conn->type) {
    case DEVICE:
	device_stop_recording(&conn->c.device);
	break;
    case PURE_DATA:
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
    case PURE_DATA:
	break;
    }

}
