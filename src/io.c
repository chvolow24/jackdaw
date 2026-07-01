#include <stdio.h>
#include <sys/stat.h>

#include "consts.h"
#include "error.h"
#include "project.h"
#include "prompt_user.h"
#include "dir.h"
#include "dot_jdaw.h"
#include "io.h"
#include "jdaw_ffmpeg.h"
#include "midi_file.h"
#include "session.h"


static int open_jdaw_file_runtime_only(const char *filepath)
{
    const char *filename = path_get_tail(filepath);
    Session *session = session_get();
    if (!session->proj_initialized) {
	error_exit("open_jdaw_file_runtime_only should not be used when project is uninitialized.\n");
    }
    char msg[255];
    snprintf(msg, 255, "Save current project \"%s\" (%s) before opening \"%s\"?", session->proj.name, "[[TODO: store project saved filepath in session]]", filename);
    const char *options[] = {"Yes", "Save As", "No", "Cancel"};
    int saveret = prompt_user("Save project?", msg, 4, options);
    if (saveret == 0 || saveret == 1) {
	fprintf(stderr, "TODO: SAVE PROJECT BEFORE CLOSING!!\n");
    }
    if (saveret == 3) {
	return 1;
    }

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

/* Return 0 on success, -1 on error */
int open_jdaw_file_starttime(const char *filepath)
{
    Session *session = session_get();
    Project new_proj;
    memset(&new_proj, '\0', sizeof(new_proj));
    session->proj_reading = &new_proj;
    int ret = jdaw_read_file(&new_proj, filepath);
    if (ret == 0) {
	session_set_proj(session, &new_proj);
	session->proj_initialized = true;
	/* TODO: handle audio format disagreements more elegantly */
	AudioConn *output = session->audio_io.playback_conn;
	if (output->open) {
	    audioconn_close(output);
	    audioconn_open(session, output);
	}
    } else {
	fprintf(stderr, "Unable to open project file at \"%s\"", filepath);
	return -1;
	/* session->proj_initialized = false; */
	/* memset(&session->proj, '\0', sizeof(Project)); */
    }
    session->proj_reading = NULL;

    for (int i=0; i<session->proj.num_timelines; i++) {
	timeline_reset_full(session->proj.timelines[i]);
    }
    return 0;

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

/*
  Returns 0 on success, -1 if no stems found.   
*/
static int open_stems_dir(const char *filepath, Timeline *tl)
{
    char **stems_paths = NULL;
    /* try_load_stems_dir validates directory, returns 0 if none found */
    int num_stems = load_stems_dir(filepath, &stems_paths);
    if (num_stems == 0) {
	return -1;
    }
    for (int i=0; i<num_stems; i++) {
	Track *track;
	if (i != 0) {
	    track = timeline_add_track(tl, i);
	} else {
	    track = tl->tracks[0];
	}
	open_audio_file(stems_paths[i], track, 0);
    }
    return 0;
}

/*
  Validates the path and returns a recognized file type.
  If valid_path_dst is provided, the validated path is copied into it.
  valid_path_dst must have room for PATH_MAX chars 
  */
IOFileType io_file_type_from_path(const char *filepath, char *valid_path_dst)
{
    char rp[PATH_MAX] = {0};
    realpath(filepath, rp);
    if (!realpath(filepath, rp)) {
	/* Not a filepath */
	return IO_FILE_INVALID_PATH;
    }
    memcpy(valid_path_dst, rp, PATH_MAX);
    struct stat s = {0};
    stat(rp, &s);
    if (S_ISDIR(s.st_mode)) {
	return IO_FILE_DIR;
    } else if (!S_ISREG(s.st_mode) && !S_ISLNK(s.st_mode)) {
	return IO_FILE_NONREG;
    }
    
    static const char *audio_file_extensions[] = {AUDIO_FILE_EXTENSIONS};
    static const char *midi_file_extensions[] = {MIDI_FILE_EXTENSIONS};
    static const char *project_file_extensions[] = {PROJECT_FILE_EXTENSIONS};
    static const char *synth_file_extensions[] = {SYNTH_FILE_EXTENSIONS};
    static const int num_audio_file_extensions = sizeof(audio_file_extensions) / sizeof(char *);
    static const int num_midi_file_extensions = sizeof(midi_file_extensions) / sizeof(char *);
    static const int num_project_file_extensions = sizeof(project_file_extensions) / sizeof(char *);
    static const int num_synth_file_extensions = sizeof(synth_file_extensions) / sizeof(char *);

    if (file_extension_in_list(rp, project_file_extensions, num_project_file_extensions)) {
	return IO_FILE_PROJ;
    } else if (file_extension_in_list(rp, midi_file_extensions, num_midi_file_extensions)) {
	return IO_FILE_MIDI;
    } else if (file_extension_in_list(rp, synth_file_extensions, num_synth_file_extensions)) {
	return IO_FILE_SYNTH;
    } else if (file_extension_in_list(rp, audio_file_extensions, num_audio_file_extensions)) {
	return IO_FILE_AUDIO;
    } else {
	return IO_FILE_EXT_UNKNOWN;
    }
}


/* Jackdaw universal file handler.

   If provided, dst_track will receive a new ClipRef at dst_tl_pos.
   
   Returns 0 on success, or negative value if error
 */
int open_file(const char *filepath, IOFileType type, Track *dst_track, int32_t dst_tl_pos)
{
    char rp[PATH_MAX] = {0};
    if (type != IO_FILE_TYPE_UNDETERMINED) {
	snprintf(rp, PATH_MAX, "%s", filepath);
    } else {
	type = io_file_type_from_path(filepath, rp);
    }

    Session *session = session_get();
    int ret = 0;
    switch (type) {
    case IO_FILE_PROJ:
	ret = open_jdaw_file_runtime_only(rp);
	break;
    case IO_FILE_MIDI:
	ret = midi_file_open(rp, false);
	break;
    case IO_FILE_SYNTH:
	if (dst_track) {
	    dst_track->synth = synth_create(dst_track);
	}
	synth_read_preset_file(rp, dst_track->synth);
	ret = 0;
	break;
    case IO_FILE_AUDIO:
	ret = open_audio_file(rp, dst_track, dst_tl_pos);
	break;
    case IO_FILE_DIR: {
	Timeline *tl = dst_track ? dst_track->tl : session->proj.timelines[0];
	open_stems_dir(rp, tl);
	ret = 0;
    }
	break;
    default:
	ret = -1;
    }
    return ret;
    /* First, validate path type */
    /* char rp[PATH_MAX] = {0}; */
    /* if (!realpath(filepath, rp)) { */
    /* 	/\* Not a filepath *\/ */
    /* 	return -1; */
    /* } */

    /* Session *session = session_get(); */
    /* struct stat s = {0}; */
    /* stat(rp, &s); */
    /* if (S_ISDIR(s.st_mode)) { */
    /* 	/\* Try loading stems dir *\/ */
    /* 	Timeline *tl = dst_track ? dst_track->tl : session->proj.timelines[0]; */
    /* 	open_stems_dir(rp, dst_track->tl); */
    /* 	return 0; */
    /* } else if (!S_ISREG(s.st_mode)) { */
    /* 	/\* Valid path, but not a file *\/ */
    /* 	return -2; */
    /* } */
    
    /* static const char *audio_file_extensions[] = {AUDIO_FILE_EXTENSIONS}; */
    /* static const char *midi_file_extensions[] = {MIDI_FILE_EXTENSIONS}; */
    /* static const char *project_file_extensions[] = {PROJECT_FILE_EXTENSIONS}; */
    /* static const char *synth_file_extensions[] = {SYNTH_FILE_EXTENSIONS}; */
    /* static const int num_audio_file_extensions = sizeof(audio_file_extensions) / sizeof(char *); */
    /* static const int num_midi_file_extensions = sizeof(midi_file_extensions) / sizeof(char *); */
    /* static const int num_project_file_extensions = sizeof(project_file_extensions) / sizeof(char *); */
    /* static const int num_synth_file_extensions = sizeof(synth_file_extensions) / sizeof(char *); */

    /* const char *filename = path_get_tail(filepath);     */
    /* int ret = 0; */
    /* if (file_extension_in_list(filepath, project_file_extensions, num_project_file_extensions)) { */
    /* 	char msg[255]; */
    /* 	snprintf(msg, 255, "Save current project \"%s\" before opening %s?", session->proj.name, filename); */
    /* 	const char *options[] = {"yes", "save as", "no"}; */
    /* 	int saveret = prompt_user("Save project?", msg, 2, options); */
    /* 	if (saveret == 0 || saveret == 1) { */
    /* 	    fprintf(stderr, "TODO: SAVE PROJECT BEFORE CLOSING!!\n"); */
    /* 	} */
    /* 	ret = open_jdaw_file(filepath); */
    /* } else if (file_extension_in_list(filepath, midi_file_extensions, num_midi_file_extensions)) { */
    /* 	ret = midi_file_open(filepath, false); */
    /* } else if (file_extension_in_list(filepath, synth_file_extensions, num_synth_file_extensions)) { */
    /* 	if (dst_track) { */
    /* 	    if (!dst_track->synth) { */
    /* 		dst_track->synth = synth_create(dst_track); */
    /* 	    } */
    /* 	    synth_read_preset_file(filepath, dst_track->synth); */
    /* 	} */
    /* } else if (file_extension_in_list(filepath, audio_file_extensions, num_audio_file_extensions)) { */
    /* 	open_audio_file(filepath, dst_track, dst_tl_pos);	 */
    /* } else { */
    /* 	/\* File extension not recognized *\/ */
    /* 	fprintf(stderr, "UNKNOWN FILE TYPE\n"); */
    /* 	ret = -1; */
    /* } */
    /* return ret; */
}



