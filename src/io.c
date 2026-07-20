#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>

#include "consts.h"
#include "error.h"
#include "loading.h"
#include "project.h"
#include "prompt_user.h"
#include "dir.h"
#include "dot_jdaw.h"
#include "io.h"
#include "jdaw_ffmpeg.h"
#include "log.h"
#include "midi_file.h"
#include "session.h"
#include "status.h"
#include "wav.h"

struct saved_dirs {
    bool initialized;
    char generic_open[PATH_MAX];
    char synth_preset[PATH_MAX];
    char export[PATH_MAX];
    char proj[PATH_MAX];
};

static struct saved_dirs saved_dirs = {0};

char *io_get_default_dir(SavedDirType type)
{
    char rp[PATH_MAX] = {0};
    if (!saved_dirs.initialized) {
	if (!realpath(INSTALL_DIR, rp)) {
	    realpath(".", rp);
	}
	snprintf(saved_dirs.generic_open, PATH_MAX, "%s", rp);
	snprintf(saved_dirs.synth_preset, PATH_MAX, "%s", rp);
	snprintf(saved_dirs.export, PATH_MAX, "%s", rp);
	snprintf(saved_dirs.proj, PATH_MAX, "%s", rp);
	saved_dirs.initialized = true;
    }
    switch(type) {
    case IO_DIR_GENERIC_OPEN:
	return saved_dirs.generic_open;
    case IO_DIR_SYNTH_PRESET:
	return saved_dirs.synth_preset;
    case IO_DIR_EXPORT:
	return saved_dirs.export;
    case IO_DIR_PROJ:
	return saved_dirs.proj;
    }
}

void io_set_default_dir(SavedDirType type, const char *path)
{
    if (!saved_dirs.initialized) {
	/* Force initialization */
	io_get_default_dir(type);
    }
    switch (type) {
    case IO_DIR_GENERIC_OPEN:
	snprintf(saved_dirs.generic_open, PATH_MAX, "%s", path);
	break;
    case IO_DIR_SYNTH_PRESET:
	snprintf(saved_dirs.synth_preset, PATH_MAX, "%s", path);
	break;
    case IO_DIR_EXPORT:
	snprintf(saved_dirs.export, PATH_MAX, "%s", path);
	break;
    case IO_DIR_PROJ:
	snprintf(saved_dirs.proj, PATH_MAX, "%s", path);
	break;
    }
}

void io_set_default_dir_all(const char *path)
{
    if (!saved_dirs.initialized) {
	io_get_default_dir(IO_DIR_GENERIC_OPEN);
    }
    snprintf(saved_dirs.generic_open, PATH_MAX, "%s", path);
    snprintf(saved_dirs.synth_preset, PATH_MAX, "%s", path);
    snprintf(saved_dirs.export, PATH_MAX, "%s", path);
    snprintf(saved_dirs.proj, PATH_MAX, "%s", path);
}


const char *io_file_get_error(IOFileType t) {
    switch (t) {
    case IO_FILE_INVALID_PATH:
	return "no such file or directory";
    case IO_FILE_NONREG:
	return "not a regular file or symbolic link";
    case IO_FILE_EXT_UNKNOWN:
	return "not a file type Jackdaw recognizes";
    default:
	return "unknown error";
    }
}

void user_global_save_project(void *nullarg);
void user_global_save_as(void *nullarg);

/* Return 1 to cancel; 0 on success; negative on error */
static int open_jdaw_file_runtime_only(const char *filepath)
{
    const char *filename = path_get_tail(filepath);
    Session *session = session_get();
    if (!session->proj_initialized) {
	error_exit("open_jdaw_file_runtime_only should not be used when project is uninitialized.\n");
    }
    if (session_proj_has_unsaved_changes()) {
        char msg[255];
        snprintf(msg, 255, "\"%s\" has unsaved changes.\n\nSave before opening \"%s\"?", session->proj.name, filename);
        const char *options[] = {"Yes", "No", "Cancel"};
        int saveret = prompt_user("Save project?", msg, 3, options, 2);
        switch (saveret) {
        case 0:
            user_global_save_project(NULL);
            /* If proj path set, can save and continue */
            if (session->proj_path_set) {
                break;
            } else {
                /* else, have to push save-as modal (breaks file open command) */
                return 2;
            }
            break;
        case 1:
            break;
        case 2:
            return 1;
        }
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
        session_set_proj_path(filepath);
	api_discard_stash();
    } else {
	const char *errstr = ret == -1 ? "parsing" : ret == -2 ? "opening" : "(unknown)";
	status_set_errstr("Error %s project file \"%s\"", errstr, filename);
	api_reset_from_stash_and_discard();
    }
    session->proj_reading = NULL;
    session_reset_window_title(false);
    
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
        session_set_proj_path(filepath);
	/* TODO: handle audio format disagreements more elegantly */
	AudioConn *output = session->audio_io.playback_conn;
	if (output->open) {
	    audioconn_close(output);
	    audioconn_open(session, output);
	}
    } else {
	/* fprintf(stderr, "Unable to open project file at \"%s\"\n", filepath); */
	return -1;
	/* session->proj_initialized = false; */
	/* memset(&session->proj, '\0', sizeof(Project)); */
    }
    session->proj_reading = NULL;
    session_reset_window_title(false);

    for (int i=0; i<session->proj.num_timelines; i++) {
	timeline_reset_full(session->proj.timelines[i]);
    }
    char *dirname = path_get_directory(filepath);
    if (dirname) {
	io_set_default_dir_all(dirname);
	/* io_set_default_dir(IO_DIR_PROJ, dirname); */
	/* snprintf(saved_dirs.synth_preset, PATH_MAX, "%s", dirname); */
	free(dirname);
    }

    return 0;

}

static NEW_EVENT_FN(undo_open_audio_file, "undo open audio file")
{
    ClipRef *cr = (ClipRef *)obj1;
    clipref_delete(cr);
}}

static NEW_EVENT_FN(redo_open_audio_file, "redo open audio file")
{
    ClipRef *cr = (ClipRef *)obj1;
    clipref_undelete(cr);
}}

static NEW_EVENT_FN(dispose_forward_open_audio_file, "")
{
    ClipRef *cr = (ClipRef *)obj1;
    clipref_destroy_no_displace(cr);
}}



/* Returns 0 on success, -1 on error */
static int open_audio_file(const char *filepath, Track *dst_track, int32_t dst_tl_pos)
{
    if (!dst_track) return -1;
    float *L, *R;
    session_set_loading_screen("Importing audio file...", NULL, true);
    int32_t length_sframes = av_open_file(filepath, &L, &R);
    if (length_sframes == 0) {
	session_loading_screen_deinit();
	return -1;
    }
    Clip *clip = clip_create(NULL, dst_track);
    session_get()->proj.active_clip_index++;
    clip->L = L;
    clip->R = R;
    clip->channels = 2;
    clip->len_sframes = length_sframes;
    clip_init_or_update_waveform(clip);
    ClipRef *cr = clipref_create(dst_track, dst_tl_pos, CLIP_AUDIO, clip);
    const char *filename = path_get_tail(filepath);
    strncpy(clip->name, filename, MAX_NAMELENGTH);
    strncpy(cr->name, filename, MAX_NAMELENGTH);

    timeline_reset(cr->track->tl, true);
    
    session_loading_screen_deinit();

    user_event_push(	    
	undo_open_audio_file,
	redo_open_audio_file,
	NULL, dispose_forward_open_audio_file,
	(void *)cr, NULL,
	(Value){0}, (Value){0}, (Value){0}, (Value){0},
	0, 0, false, false);

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
    char ldscr[255];

    /* session_set_loading_screen("Loading stems...", NULL, true); */
    for (int i=0; i<num_stems; i++) {
	snprintf(ldscr, 255, "Loading stems (%d / %d)", i + 1, num_stems);
	if (i == 0) {
	    session_set_loading_screen(ldscr, NULL, true);
	} else {
	    session_loading_screen_set_title(ldscr);
	}
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
    if (valid_path_dst)
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
   
   Return an IOFileType indicating the type of file opened,
   or the class of error if unsuccessful
   
 */
IOFileType io_open_file(const char *filepath, IOFileType type, Track *dst_track, int32_t dst_tl_pos)
{
    char rp[PATH_MAX] = {0};
    if (type != IO_FILE_TYPE_UNDETERMINED) {
	snprintf(rp, PATH_MAX, "%s", filepath);
    } else {
	type = io_file_type_from_path(filepath, rp);
    }

    Session *session = session_get();
    IOFileType ret = type;
    switch (type) {
    case IO_FILE_PROJ:
	if (open_jdaw_file_runtime_only(rp) < 0) {
	    ret = IO_FILE_ERROR;
	}
	break;
    case IO_FILE_MIDI:
	if (midi_file_open(rp, false) < 0) {
	    ret = IO_FILE_ERROR;
	}
	break;
    case IO_FILE_SYNTH:
	if (dst_track) {
	    if (!dst_track->synth)
		dst_track->synth = synth_create(dst_track);
	    if (synth_read_preset_file(rp, dst_track->synth) < 0) {
		ret = IO_FILE_ERROR;
	    } else {
                snprintf(dst_track->synth->preset_filepath, PATH_MAX, "%s", rp);
                fprintf(stderr, "Synth preset filepath: %s\n", dst_track->synth->preset_filepath);
                /* dst_track->synth-> */
            }
	}
	break;
    case IO_FILE_AUDIO:
	if (open_audio_file(rp, dst_track, dst_tl_pos) < 0) {
	    ret = IO_FILE_ERROR;
	}
	break;
    case IO_FILE_DIR: {
	Timeline *tl = dst_track ? dst_track->tl : session->proj.timelines[0];
	if (open_stems_dir(rp, tl) < 0) {
	    ret = IO_FILE_ERROR;
	}
    }
	break;
    default:
	return type;
	break;
    }
    if (ret < 0) {
	log_tmp(LOG_ERROR, "An error occurred while opening file %s\n", filepath);
    } else {
	if (type == IO_FILE_SYNTH) {
	    char *dirname = path_get_directory(rp);
	    if (dirname) {
		io_set_default_dir(IO_DIR_SYNTH_PRESET, dirname);
		/* snprintf(saved_dirs.synth_preset, PATH_MAX, "%s", dirname); */
		free(dirname);
	    }
	} else {
	    char *dirname = path_get_directory(rp);
	    if (dirname) {
		io_set_default_dir(IO_DIR_GENERIC_OPEN, dirname);
		if (type == IO_FILE_PROJ) {
		    io_set_default_dir(IO_DIR_PROJ, dirname);
		}
		free(dirname);
	    }
	} 
    }
    return ret;
}

const char *io_file_type_str(IOFileType t)
{
    switch (t) {
    case IO_FILE_PROJ:
        return "project";
    case IO_FILE_MIDI:
        return "midi";
    case IO_FILE_SYNTH:
        return "synth preset";
    case IO_FILE_AUDIO:
        return "audio";
    case IO_FILE_DIR:
        return "directory";
    case IO_FILE_INVALID_PATH:
        return "invalid path";
    case IO_FILE_NONREG:
        return "not a regular file or directory";
    case IO_FILE_EXT_UNKNOWN:
        return "file extension unknown";
    case IO_FILE_TYPE_UNDETERMINED:
        return "file type undetermined";
    case IO_FILE_NO_OVERWRITE:
        return "user cancelled overwrite";
    case IO_FILE_ERROR:
        return "file IO error";
    case NUM_IO_FILE_TYPES:
        return "";
    }
}


IOFileType io_write_file(const char *filepath, IOFileType type, bool force_allow_overwrite, void *object_to_write)
{
    if (!IO_FILE_TYPE_OK(type) || type == IO_FILE_DIR) {
	log_tmp(LOG_ERROR, "io_save_file received file type %d\n", type);
	return IO_FILE_TYPE_UNDETERMINED;
    }
    path_get_directory(filepath);
    IOFileType ret = type;

    char filepath_mutable[PATH_MAX];
    snprintf(filepath_mutable, PATH_MAX, "%s", filepath);
    char *dir = NULL;
    char *filename = NULL;
    dir = strdup(dirname(filepath_mutable));
    char validated_dir[PATH_MAX] = {0};
    if (io_file_type_from_path(dir, validated_dir) != IO_FILE_DIR) {
	ret = IO_FILE_ERROR;
	goto cleanup_and_ret;
    }
    snprintf(filepath_mutable, PATH_MAX, "%s", filepath);
    filename = strdup(basename(filepath_mutable));
    char full_path[PATH_MAX];    
    snprintf(full_path, PATH_MAX, "%s/%s", validated_dir, filename);

    if (!force_allow_overwrite) {
        bool file_exists = false;
        struct stat s = {0};
        if (stat(full_path, &s) == 0) {
            file_exists = true;
        }
        const char *overwrite_opts[] = {
            "Yes, overwrite",
            "No, cancel"
        };
        if (file_exists) {
            char hdr[255];
            snprintf(hdr, 255, "File \"%s\" already exists. Overwrite it?", filename);
            int sel = prompt_user("Overwrite?", hdr, 2, overwrite_opts, 1);
            if (sel == 1) {
                ret = IO_FILE_NO_OVERWRITE;
                goto cleanup_and_ret;
            }
        }
    }
    
    /* TODO: Handle errors and return info to caller */
    switch (type) {
    case IO_FILE_PROJ:
	if (jdaw_write_project(full_path) == 0) {
            session_set_proj_path(full_path);
            session_set_proj_name(filename); 
        } else {
            ret = IO_FILE_ERROR;
        }
	break;
    case IO_FILE_MIDI:
	break;
    case IO_FILE_SYNTH:
	synth_write_preset_file(full_path, object_to_write);
	break;
    case IO_FILE_AUDIO:
	/* TODO: replace with universal FFMPEG file writer */
	wav_write_mixdown(full_path);
	break;
    default:
	break;	
    }
    if (IO_FILE_TYPE_OK(type)) {
        status_set_alertstr("Wrote %s file at %s", io_file_type_str(type), full_path);
    } else if (type == IO_FILE_NO_OVERWRITE) {
        status_set_alertstr("File write canceled");
    }
cleanup_and_ret:
    if (dir) free(dir);
    if (filename) free(filename);
    return ret;

}
