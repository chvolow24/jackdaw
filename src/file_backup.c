/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "loading.h"

bool file_exists(const char *filepath)
{
    return access(filepath, F_OK) == 0;
}

int file_copy(const char *read_filepath, const char *write_filepath)
{

    if (!file_exists(read_filepath)) {
	fprintf(stderr, "Error: file does not exist.\n");
	return 1;
    }
    FILE *fr = fopen(read_filepath, "rb");
    if (!fr) {
	fprintf(stderr, "Error in file copy: unable to open read filepath \"%s\"\n",  read_filepath);
	return 1;
    }

    fseek(fr, 0, SEEK_END);
    long unsigned file_size_bytes = ftell(fr);
    rewind(fr);


    FILE *fw = fopen(write_filepath, "wb");
    if (!fw) {
	fprintf(stderr, "Error in file copy: unable to open write filepath \"%s\"\n", write_filepath);
	return 1;
    }

    long int byte_i = 0;
    long int byte_mod = file_size_bytes / 100;
    int c;
    while ((c = fgetc(fr)) != EOF) {
	/* fprintf(stderr, "Byte i: %lu, mod: %lu fs: %lu", byte_i, byte_i % byte_mod, file_size_bytes); */
	if (byte_i % byte_mod == 0) {
	    session_loading_screen_update(NULL, (double)byte_i / file_size_bytes);
	}
	byte_i++;
	fputc(c, fw);
    }
    
    fclose(fr);
    fclose(fw);
    return 0;
}

void file_backup(const char *filepath)
{

    session_set_loading_screen("Overwrite backup", "Backing up existing file...", false);
    const char *bak = ".bak";
    int buflen = strlen(filepath) + strlen(bak) + 1;
    char buf[buflen];
    snprintf(buf, buflen, "%s%s\n", filepath, bak);
    fprintf(stderr, "Backing up file at: %s\n", buf);
    if (rename(filepath, buf) != 0) {
	perror("Rename error");
    }
    
    /* if (file_copy(filepath, (const char *)buf) != 0) { */
    /* 	session_loading_screen_deinit(proj); */
    /* 	fprintf(stderr, "Error: failed to backup file \"%s\".", filepath); */
    /* } */
    session_loading_screen_deinit();
}
