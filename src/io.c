#include <stdio.h>
#include "consts.h"
#include "project.h"
#include "prompt_user.h"
#include "dir.h"
#include "dot_jdaw.h"
#include "jdaw_ffmpeg.h"
#include "midi_file.h"
#include "session.h"

static int open_jdaw_file(const char *filepath)
{
    Session *session = session_get();
    if (session->playback.recording) transport_stop_recording();
    else if (session->playback.playing) transport_stop_playback();
    /* Wait for playback callback to exit */
    audioconn_close(session->audio_io.playback_conn);
    /* api_quit(); */
    api_stash_current();
    Project new_proj;
    memset(&new_proj, '\0', sizeof(new_proj));
    session->proj_reading = &new_proj;
    int ret = jdaw_read_file(&new_proj, filepath);
    if (ret == 0) {
	session_set_proj(session, &new_proj);
	api_discard_stash();
    } else {
	status_set_errstr("Error opening jdaw project");
	api_reset_from_stash_and_discard();
	return ret;
    }
    session->proj_reading = NULL;
    return ret;
}

static int open_audio_file(const char *filepath, Track *dst_track, int32_t dst_tl_pos)
{
    if (!dst_track) return -1;
    float *L, *R;
    int32_t length_sframes = av_open_file(filepath, &L, &R);
    if (length_sframes == 0) {
	return -1;
    }
    Clip *clip = clip_create(NULL, dst_track);
    clip->L = L;
    clip->R = R;
    clip->channels = 2;
    clip->len_sframes = length_sframes;
    clip_init_or_update_waveform(clip);
    ClipRef *cr = clipref_create(dst_track, dst_tl_pos, CLIP_AUDIO, clip);
    timeline_reset(cr->track->tl, true);
    return 0;
}

/* Jackdaw universal file handler.

   If provided, dst_track will receive a new ClipRef at dst_tl_pos.
   
   Returns 0 on success, value < 0  on error.   
 */
int open_file(const char *filepath, Track *dst_track, int32_t dst_tl_pos)
{   
    static const char *audio_file_extensions[] = {AUDIO_FILE_EXTENSIONS};
    static const char *midi_file_extensions[] = {MIDI_FILE_EXTENSIONS};
    static const char *project_file_extensions[] = {PROJECT_FILE_EXTENSIONS};
    static const char *synth_file_extensions[] = {SYNTH_FILE_EXTENSIONS};
    static const int num_audio_file_extensions = sizeof(audio_file_extensions) / sizeof(char *);
    static const int num_midi_file_extensions = sizeof(midi_file_extensions) / sizeof(char *);
    static const int num_project_file_extensions = sizeof(project_file_extensions) / sizeof(char *);
    static const int num_synth_file_extensions = sizeof(synth_file_extensions) / sizeof(char *);

    Session *session = session_get();
    const char *filename = path_get_tail(filepath);
    
    int ret = 0;
    if (file_extension_in_list(filepath, project_file_extensions, num_project_file_extensions)) {
	char msg[255];
	snprintf(msg, 255, "Save current project \"%s\" before opening %s?", session->proj.name, filename);
	const char *options[] = {"yes", "save as", "no"};
	int saveret = prompt_user("Save project?", msg, 2, options);
	if (saveret == 0 || saveret == 1) {
	    fprintf(stderr, "TODO: SAVE PROJECT BEFORE CLOSING!!\n");
	}
	ret = open_jdaw_file(filepath);
    } else if (file_extension_in_list(filepath, midi_file_extensions, num_midi_file_extensions)) {
	ret = midi_file_open(filepath, false);
    } else if (file_extension_in_list(filepath, synth_file_extensions, num_synth_file_extensions)) {
	if (dst_track) {
	    if (!dst_track->synth) {
		dst_track->synth = synth_create(dst_track);
	    }
	    synth_read_preset_file(filepath, dst_track->synth);
	}
    } else if (file_extension_in_list(filepath, audio_file_extensions, num_audio_file_extensions)) {
	open_audio_file(filepath, dst_track, dst_tl_pos);	
    } else {
	/* File extension not recognized */
	fprintf(stderr, "UNKNOWN FILE TYPE\n");
	ret = -1;
    }
    return ret;

}
