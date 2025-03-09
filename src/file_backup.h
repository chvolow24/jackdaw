#ifndef JDAW_FILE_BACKUP_H
#define JDAW_FILE_BACKUP_H

#include <stdbool.h>

bool file_exists(const char *filepath);
void file_backup(const char *filepath);

#endif
