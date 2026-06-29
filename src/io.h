#ifndef JDAW_IO_H
#define JDAW_IO_H

#include <stdio.h>
#include "project.h"

/* Jackdaw universal file handler.

   If provided, dst_track will receive a new ClipRef at dst_tl_pos.
   
   Returns 0 on success, value < 0  on error.   
 */
int open_file(const char *filepath, Track *dst_track, int32_t dst_tl_pos);
#endif
