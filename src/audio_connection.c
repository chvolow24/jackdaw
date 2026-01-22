/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <pthread.h>
/* #include <semaphore.h> */
#include "audio_connection.h"
#include "consts.h"
#include "error.h"
#include "log.h"
#include "project.h"
#include "pure_data.h"
#include "session.h"
#include "status.h"


/* #define DEVICE_BUFLEN_SECONDS 2 /\* TODO: reduce, and write to clip during recording *\/ */
#define DEVICE_BUFLEN_CHUNKS 1024

#define PD_BUFLEN_CHUNKS 1024


int audio_io_get_devices(Session *session, int iscapture)
{
    char *default_dev_name = NULL;
    SDL_AudioSpec default_dev_spec;
    bool has_default_dev = true;
    if (SDL_GetDefaultAudioInfo(&default_dev_name, &default_dev_spec, iscapture) == 0) {
	has_default_dev = true;
    }
    int num_devices = SDL_GetNumAudioDevices(iscapture);
    AudioDevice **dst_list;
    if (iscapture) {
	session->audio_io.num_record_devices = num_devices;;
	dst_list = session->audio_io.record_devices;
    } else {
	session->audio_io.num_playback_devices = num_devices;
	dst_list = session->audio_io.playback_devices;
    }

    for (int i=0; i<num_devices; i++) {
	SDL_AudioSpec spec = {0};
	AudioDevice *dev = calloc(1, sizeof(AudioDevice));
	dst_list[i] = dev;
	const char *name = SDL_GetAudioDeviceName(i, iscapture);
	if (!name) {
	    error_exit("Unable to get %s audio device name at index %d\n", iscapture ? "record" : "playback", i);
	}
	if (has_default_dev && strcmp(name, default_dev_name) == 0) {
	    dev->is_default = true;
	    spec = default_dev_spec;
	} else {
	    dev->is_default = false;
	    if (SDL_GetAudioDeviceSpec(i, iscapture, &spec) != 0) {
		error_exit("Unable to get %s audio device spec at index %d\n", iscapture ? "record" : "playback", i);
	    }
	}
	snprintf(dev->name, MAX_DEV_NAMELENGTH, "%s", name);
	dev->index = i;
	dev->spec = spec;
	
    }
    if (default_dev_name) {
	free(default_dev_name);
    }
    return num_devices;    
}

/* Creates audio conns with default settings and channel cfg */
int audio_io_get_connections(Session *session, int iscapture)
{
    audio_io_get_devices(session, iscapture);
    AudioConn **conn_list;
    if (iscapture) {
	conn_list = session->audio_io.record_conns;
    } else {
	conn_list = session->audio_io.playback_conns;
    }
    int num_devices = iscapture ? session->audio_io.num_record_devices : session->audio_io.num_playback_devices;
    int *conn_index = iscapture ? &session->audio_io.num_record_conns : &session->audio_io.num_playback_conns;
    int *default_conn_index = iscapture ? &session->audio_io.default_record_conn_index : &session->audio_io.default_playback_conn_index;
    AudioDevice **device_list = iscapture ? session->audio_io.record_devices : session->audio_io.playback_devices;
    for (int device_i=0; device_i<num_devices; device_i++) {
	AudioDevice *dev = device_list[device_i];
	int channel_i = 0;
	while (channel_i < dev->spec.channels) {
	    AudioConn *conn = calloc(sizeof(AudioConn), 1);
	    if (dev->spec.channels - channel_i >= 2) {
		conn->channel_cfg.L_src = channel_i;
		conn->channel_cfg.R_src = channel_i + 1;
		dev->channel_dsts[channel_i] = (struct channel_dst){conn, 0};
		dev->channel_dsts[channel_i + 1] = (struct channel_dst){conn, 1};
		snprintf(conn->name, MAX_CONN_NAMELENGTH, "%s (ch. %d, %d)", dev->name, channel_i + 1, channel_i + 2);
	    } else {
		conn->channel_cfg.L_src = channel_i;
		conn->channel_cfg.R_src = -1;
		dev->channel_dsts[channel_i] = (struct channel_dst){conn, 0};
		snprintf(conn->name, MAX_CONN_NAMELENGTH, "%s (ch. %d)", dev->name, channel_i + 1);
	    }
	    conn->obj = dev;
	    conn->available = true;
	    conn->iscapture = iscapture;
	    conn->index = *conn_index;
	    if (dev->is_default && channel_i == 0) {
		*default_conn_index = conn->index;
	    }
	    conn_list[*conn_index] = conn;
	    (*conn_index)++;
	    channel_i += 2;   
	}
    }	

    if (iscapture) {
	AudioConn *pd = calloc(sizeof(AudioConn), 1);
	pd->type = PURE_DATA;
	strcpy(pd->name, "Pure data");
	pd->index = num_devices;
	pd->iscapture = iscapture;
	pd->available = true;
	pd->obj = &session->audio_io.pd_conn;
	conn_list[*conn_index] = pd;	    
	(*conn_index)++;
	    
	AudioConn *jdaw = calloc(sizeof(AudioConn), 1);
	jdaw->type = JACKDAW;
	strcpy(jdaw->name, "Jackdaw out");
	jdaw->index = num_devices;
	jdaw->iscapture = iscapture;
	jdaw->available = true;
	pd->obj = &session->audio_io.jdaw_conn;
	conn_list[*conn_index] = jdaw;
	(*conn_index)++;
    }
    return (*conn_index);
}


void transport_playback_callback(void *user_data, uint8_t *stream, int len);
void transport_record_callback(void *user_data, uint8_t *stream, int len);

int audioconn_open(Session *session, AudioConn *conn)
{
    if (conn->open) {
	log_tmp(LOG_WARN, "Audio conn \"%s\" already open\n", conn->name);
	return 1;
    }
    log_tmp(LOG_INFO, "Call to open audio conn \"%s\"\n", conn->name);
    switch (conn->type) {
    case DEVICE: {
	AudioDevice *device = conn->obj;
	if (device->open) {
	    log_tmp(LOG_WARN, "Audio device \"%s\" already open\n", device->name);
	    return 1;
	}

	SDL_AudioSpec obtained;
	SDL_zero(obtained);
	
	/* SDL_zero(device->spec); */
	/* Project determines high-level audio settings */
	if (session->proj_initialized) {
	    device->spec.format = session->proj.fmt;
	    device->spec.samples = session->proj.chunk_size_sframes;
	    device->spec.freq = session_get_sample_rate();
	    if (!conn->iscapture) { // if capture dev, open with default num channels
		device->spec.channels = session->proj.channels; // else open with 2 channels
	    }
	} else {
	    device->spec.format = DEFAULT_SAMPLE_FORMAT;
	    device->spec.samples = DEFAULT_AUDIO_CHUNK_LEN_SFRAMES;
	    device->spec.freq = DEFAULT_SAMPLE_RATE;
	    if (!conn->iscapture) { // if capture dev, open with default num channels
		device->spec.channels = 2; // else open with 2 channels
	    }
	}
	device->spec.callback = conn->iscapture ? transport_record_callback : transport_playback_callback;
	device->spec.userdata = device; /* conn accessible via device channel_dsts */

	if ((device->id = SDL_OpenAudioDevice(device->name, conn->iscapture, &(device->spec), &(obtained), 0)) > 0) {
	    device->spec = obtained;
	    device->open = true;
	    conn->open = true;
	    log_tmp(LOG_INFO, "Successfully opened audio device \"%s\"\n", device->name);
	} else {
	    conn->open = false;
	    device->open = false;
	    log_tmp(LOG_ERROR, "Could not open audio device \"%s\": (SDL:) %s\n", device->name, SDL_GetError());
	    return 1;
	}

	if (conn->iscapture) {
	    device->rec_buf_len_samples = DEVICE_BUFLEN_CHUNKS * session->proj.chunk_size_sframes * device->spec.channels;
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
	PdConn *pdconn = conn->obj;
	if (conn->iscapture) {
	    if (!pdconn->rec_buffer_L) {
		pdconn->rec_buf_len_sframes = PD_BUFLEN_CHUNKS * session->proj.chunk_size_sframes;
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
	    JDAWConn *jconn = conn->obj;
	    jconn->rec_buf_len_sframes = PD_BUFLEN_CHUNKS * session->proj.chunk_size_sframes;
	    if (!jconn->rec_buffer_L) jconn->rec_buffer_L = malloc(sizeof(float) * jconn->rec_buf_len_sframes);
	    if (!jconn->rec_buffer_R) jconn->rec_buffer_R = malloc(sizeof(float) * jconn->rec_buf_len_sframes);
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
    if (conn->open) {
	audioconn_close(conn);
    }
    free(conn);
}

void audio_device_destroy(AudioDevice *dev)
{
    if (dev->rec_buffer) {
	free(dev->rec_buffer);
    }
    free(dev);
}

void jdaw_conn_destroy(JDAWConn *jconn)
{
    if (jconn->rec_buffer_L) {
	free(jconn->rec_buffer_L);
    }
    if (jconn->rec_buffer_R) {
	free(jconn->rec_buffer_R);
    }
}

static void device_close(AudioDevice *device)
{
    if (device->rec_buffer) {
	free(device->rec_buffer);
	device->rec_buffer = NULL;
    }
    /* fprintf(stdout, "CLOSING device %s, id: %d\n",device->name, device->id); */
    SDL_CloseAudioDevice(device->id);
    device->open = false;
    device->playing = false;
    device->id = 0;
}

void jdaw_conn_close(JDAWConn *jconn)
{
    if (jconn->rec_buffer_L) {
	free(jconn->rec_buffer_L);
    }
    if (jconn->rec_buffer_R) {
	free(jconn->rec_buffer_R);
    }
}

void audioconn_close(AudioConn *conn)
{
    if (!conn->open) {
	log_tmp(LOG_WARN, "AudioConn \"%s\" already closed\n", conn->name);
	return;
    }
    log_tmp(LOG_INFO, "Call to close AudioConn \"%s\"\n", conn->name);
    conn->open = false;
    switch (conn->type) {
    case DEVICE:
	device_close(conn->obj);
	break;
    case PURE_DATA:
	break;
    case JACKDAW:
	jdaw_conn_close(conn->obj);
	break;
    }    
}

static void device_start_playback(AudioDevice *dev)
{
    if (!dev->playing) {
	SDL_PauseAudioDevice(dev->id, 0);
	dev->playing = true;
    }
}


void audioconn_start_playback(AudioConn *conn)
{    
    if (!conn->available) {
	status_set_errstr("No audio can be played through this device.");
	return;
    }
    if (conn->playing) return;
    switch (conn->type) {
    case DEVICE:
	device_start_playback(conn->obj);
	break;
    case PURE_DATA:
	break;
    case JACKDAW:
	break;
    }
    conn->playing = true;
}

static void device_stop_playback(AudioDevice *dev)
{
    dev->request_close = true;
    /* SDL_PauseAudioDevice(dev->id, 1); */
}

void audioconn_stop_playback(AudioConn *conn)
{
    if (!conn->playing) return;
    switch (conn->type) {
    case DEVICE:
	device_stop_playback(conn->obj);
	break;
    case PURE_DATA:
	break;
    case JACKDAW:
	break;
    }
    conn->playing = false;
}

static void device_stop_recording(AudioDevice *dev)
{
    fprintf(stderr, "STOP RECORDING DEVICE: %s\n", dev->name);
    SDL_PauseAudioDevice(dev->id, 1);
    /* dev->write_bufpos_samples = 0; */
    /* device_close(dev); */
}

void audioconn_stop_recording(AudioConn *conn)
{
    fprintf(stderr, "AUDIOCONN STOP RECORDING: %s\n", conn->name);
    switch (conn->type) {
    case DEVICE:
	device_stop_recording(conn->obj);
	break;
    case PURE_DATA:
	break;
    case JACKDAW:
	break;
    }
}

static void device_start_recording(AudioDevice *dev)
{
    SDL_PauseAudioDevice(dev->id, 0);
}

void audioconn_start_recording(AudioConn *conn)
{
    switch (conn->type) {
    case DEVICE:
	device_start_recording(conn->obj);
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
    /* Requires full rewrite */
}

void audioconn_reset_chunk_size(AudioConn *c, uint16_t new_chunk_size)
{
    if (c->open) {
	audioconn_close(c);
    }
    switch (c->type) {
    case DEVICE:
	((AudioDevice *)c->obj)->spec.samples = new_chunk_size;
	break;
    default:
	break;
    }
}

void session_init_audio_conns(Session *session)
{
    /* session->audio_io.num_playback_conns = audio_io_get_connections(session, 0); */
    /* session->audio_io.num_record_conns = audio_io_get_connections(session, 1); */
    /* session->audio_io.playback_conn = session->audio_io.playback_conns[0]; */
    audio_io_get_connections(session, 0);
    audio_io_get_connections(session, 1);
    /* session->audio_io.playback_conn =  */
    AudioConn *playback_conn = session->audio_io.playback_conns[session->audio_io.default_playback_conn_index];
    if (playback_conn->available && audioconn_open(session, playback_conn) != 0) {
	log_tmp(LOG_ERROR, "Failed to open default playback conn \"%s\" at init time.\n", playback_conn->name);
    }
    session->audio_io.playback_conn = playback_conn;
}

#include "menu.h"
extern Window *main_win;

static void select_out_onclick(void *arg)
{
    Session *session = session_get();
    int index = *((int *)arg);
    if (session->playback.playing) {
	transport_stop_playback();
	timeline_play_speed_set(0.0);
    }
    audioconn_close(session->audio_io.playback_conn);
    session->audio_io.playback_conn = session->audio_io.playback_conns[index];
    if (audioconn_open(session, session->audio_io.playback_conn) != 0) {
	fprintf(stderr, "Error: failed to open default audio conn \"%s\". More info: %s\n", session->audio_io.playback_conn->name, SDL_GetError());
	status_set_errstr("Error: failed to open default audio conn \"%s\"");
    }
    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_out_value");
    textbox_set_value_handle(((Button *)el->component)->tb, session->audio_io.playback_conn->name);
    window_pop_menu(main_win);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
    /* window_pop_mode(main_win); */
}

void session_set_default_out(void *nullarg)
{
    Session *session = session_get();
    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "panel_out_value");
    SDL_Rect *rect = &((Button *)el->component)->tb->layout->rect;
    Menu *menu = menu_create_at_point(rect->x, rect->y);
    MenuColumn *c = menu_column_add(menu, "");
    MenuSection *sc = menu_section_add(c, "");
    for (int i=0; i<session->audio_io.num_playback_conns; i++) {
	AudioConn *conn = session->audio_io.playback_conns[i];
	/* fprintf(stdout, "Conn index: %d\n", conn->index); */
	if (conn->available) {
	    menu_item_add(
		sc,
		conn->name,
		" ",
		select_out_onclick,
		&(conn->index));
	}
    }
    menu_add_header(menu,"", "Select the default audio output.\n\n'n' to select next item; 'p' to select previous item.");
    /* menu_reset_layout(menu); */
    window_add_menu(main_win, menu);
}
