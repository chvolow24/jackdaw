/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    file_backup.h

    * rudimentary interface for backing up binary files
    * backups are executed every time a save is executed on an existing file
 *****************************************************************************************************************/


#ifndef JDAW_FILE_BACKUP_H
#define JDAW_FILE_BACKUP_H

#include <stdbool.h>

bool file_exists(const char *filepath);
void file_backup(const char *filepath);

#endif
