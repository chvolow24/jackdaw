#ifndef JDAW_TMP_H
#define JDAW_TMP_H

const char* system_tmp_dir();

/* 'name' is filename, including extension. Returns the full filepath, which must be freed when done */
char *create_tmp_file(const char *name);

#endif
