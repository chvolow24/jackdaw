/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    dot_jdaw.h

    * Define the .jdaw file type and provide functions for saving and opening .jdaw files
 *****************************************************************************************************************/


#ifndef JDAW_DOT_JDAW_H
#define JDAW_DOT_JDAW_H

#include "project.h"

/* Writes a new .jdaw file, returns 0 on success <0 on error */
int jdaw_write_project(FILE *f);

/* File has already been validated as a .jdaw file (io.c) */
int jdaw_read_file(Project *dst, FILE *f);

/* Used by synth.c */
void jdaw_write_effect_chain_external(FILE *f, EffectChain *ec);
int jdaw_read_effect_chain_external(FILE *f, Project *proj, EffectChain *ec, APINode *api_node, const char *obj_name, int32_t chunk_len_sframes);
    
/* void write_jdaw_file(const char *path); */

/* Open a .jdaw file at the directory pointed to by 'path', and return a pointer to the built Project struct */
/* Project *open_jdaw_file(const char *path); */


#endif
