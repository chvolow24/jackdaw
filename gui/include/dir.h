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
    DirPath *dp;
    Layout *layout;
    bool show_dirs;
    bool show_files;
    Textbox *instruction;
    Textbox *tb_current_path;
    char line_text[32768];
    TextArea *lines;
    /* Menu *entries; */
    /* TextArea *lines; */
    /* uint8_t num_lines; */
    uint8_t current_line;
} DirNav;

typedef struct dirpath {
    char path[MAX_PATHLEN];
    DirPath *dirs[MAX_DIRS];
    uint8_t num_dirs;
    FilePath *files[MAX_FILES];
    uint8_t num_files;
} DirPath;

typedef struct filepath {
    char path[MAX_PATHLEN];
} FilePath;


DirNav *dirnav_create(const char *dir_name, Layout *lt, bool show_dirs, bool show_files);
void dirnav_draw(DirNav *dn);

#endif
