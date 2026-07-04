#ifndef JDAW_IO_H
#define JDAW_IO_H

#include <stdio.h>
#include "project.h"

/* Valid input files */
typedef enum io_file_type {
    IO_FILE_PROJ,
    IO_FILE_MIDI,
    IO_FILE_SYNTH,
    IO_FILE_AUDIO,
    IO_FILE_DIR, /* e.g. stems dir */
    IO_FILE_INVALID_PATH,
    IO_FILE_NONREG, /* not a regular file, symbolic link, or directory */
    IO_FILE_EXT_UNKNOWN,
    IO_FILE_TYPE_UNDETERMINED, /* used as function argument when path has not been checked */
    IO_FILE_ERROR, /* when a file type is known, but parsing fails */
    NUM_IO_FILE_TYPES
} IOFileType;

#define IO_FILE_TYPE_OK(type) (type < IO_FILE_INVALID_PATH)

const char *io_file_get_error(IOFileType t);

IOFileType io_file_type_from_path(const char *filepath, char *valid_path_dst);

/* Save directory paths for opening / saving files */
typedef enum saved_dir_type {
    IO_DIR_GENERIC_OPEN,
    IO_DIR_SYNTH_PRESET,
    IO_DIR_EXPORT,
    IO_DIR_PROJ
} SavedDirType;

/* Jackdaw universal file handler.

   If provided, dst_track will receive a new ClipRef at dst_tl_pos.

   Returns the IOFileType of the filepath.
 */
IOFileType open_file(const char *filepath, IOFileType type, Track *dst_track, int32_t dst_tl_pos);

/* Use when jackdaw invoked on cmd line with project file arg.
    Returns 0 on success, <0 on error. */
int open_jdaw_file_starttime(const char *filepath);

/* Get the default directory for a particular file operation (e.g. generic open) */
char *io_get_default_dir(SavedDirType type);

/* Set the default directory for a particular file operation (e.g. generic open) */
void io_set_default_dir(SavedDirType type, const char *path);

#endif
