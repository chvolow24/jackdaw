#ifndef JDAW_GUI_DIR
#define JDAW_GUI_DIR

#include "color.h"
#include "textbox.h"

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define MAX_DIRS 255
#define MAX_FILES 255
#define MAX_PATHLEN 255
#define MAX_DIRNAV_TXTLEN

typedef struct dirpath DirPath;
typedef struct filepath FilePath;

typedef struct dirnav {
    DirPath *dirpath;
    Layout *layout;
    bool show_dirs;
    bool show_files;
    Textbox *instruction;
    Textbox *current_path;
    /* char line_text[32768]; */
    /* TextArea *lines; */
    TextLines *lines;
    uint16_t num_lines;
    /* Menu *entries; */
    /* TextArea *lines; */
    /* uint8_t num_lines; */
    uint16_t current_line;
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


DirNav *dirnav_create(const char *dir_name, Layout *lt, bool show_dirs, bool show_files);
void dirnav_draw(DirNav *dn);
void dirnav_destroy(DirNav *dn);
void dirnav_next(DirNav *dn);
void dirnav_previous(DirNav *dn);
void dirnav_select(DirNav *dn);

#endif
