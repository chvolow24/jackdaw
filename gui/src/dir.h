#ifndef JDAW_GUI_DIR
#define JDAW_GUI_DIR

#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "color.h"
#include "textbox.h"

#define MAX_DIRS 255
#define MAX_FILES 255
#define MAX_PATHLEN 255
#define MAX_DIRNAV_TXTLEN

typedef struct dirpath DirPath;
typedef struct filepath FilePath;
typedef struct dirnav DirNav;
typedef struct dirnav {
    DirPath *dirpath;
    Layout *layout;
    /* bool show_dirs; */
    /* bool show_files; */
    Textbox *instruction;
    Textbox *current_path_tb;
    char current_path_str[MAX_PATHLEN];
    TextLines *lines;
    uint16_t num_lines;
    uint16_t current_line;
    int (*dir_to_tline_filter)(void *item, void *x_arg);
    void (*file_select_action)(DirNav *self, DirPath *dp);
} DirNav;

typedef struct dirpath {
    char path[MAX_PATHLEN];
    uint8_t type; /* E.g. DT_DIR, DT_REG */
    DirPath *entries[MAX_DIRS];
    uint8_t num_entries;
    bool hidden;
    /* uint8_t num_files; */
    /* uint8_t num_dirs; */
    /* FilePath *files[MAX_FILES]; */
    /* uint8_t num_files; [[*/
} DirPath;

typedef struct filepath {
    char path[MAX_PATHLEN];
} FilePath;


char *path_get_tail(char *pathname);
DirNav *dirnav_create(const char *dir_name, Layout *lt, int (*dir_to_tline_filter)(void *dp_v, void *dn_v));
void dirnav_draw(DirNav *dn);
void dirnav_destroy(DirNav *dn);
TLinesItem *dirnav_select_item(DirNav *dn, uint16_t i);
void dirnav_next(DirNav *dn);
void dirnav_previous(DirNav *dn);
void dirnav_select(DirNav *dn);
void dirnav_triage_click(DirNav *dn, SDL_Point *mousep);

#endif
