#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir.h"
#include "textbox.h"

extern Window *main_win;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;


/* static char *dir_get_homepath() */
/* { */
/*     uid_t uid = getuid(); */
/*     struct passwd *pw = getpwuid(uid); */
/*     return pw->pw_dir; */
/* } */

char *filetype(uint8_t t)
{
    switch(t) {
    case (DT_UNKNOWN):
	return "DT_UNKNOWN";
    case (DT_FIFO):
	return "DT_FIFO";
    case (DT_CHR):
	return "DT_CHR";
    case (DT_DIR):
	return "DT_DIR";
    case (DT_BLK):
	return "DT_BLK";
    case (DT_REG):
	return "DT_REG";
    case (DT_LNK):
	return "DT_LNK";
    case (DT_SOCK):
	return "DT_SOCK";
    case (DT_WHT):
	return "DT_WHT";
    };
    return 0;
}

/* void print_dir_contents(char *dir_name, int indent) */
/* { */
/*     if (indent > 3) { */
/* 	return; */
/*     } */
/*     DIR *dir = opendir(dir_name); */
/*     struct dirent *tdir; */
/*     while ((tdir = readdir(dir))) { */
/* 	fprintf(stdout, "%*sName: %s\n", indent, "", tdir->d_name); */
/* 	if (tdir->d_type == DT_DIR && strcmp(".", tdir->d_name) != 0 && strcmp("..", tdir->d_name) != 0) { */
/* 	    print_dir_contents(tdir->d_name, indent+1); */
/* 	} */
/*     } */
/* } */


static int path_updir_name(char *pathname)
{
    int ret = 0;
    char *current = NULL;
    char *mov = pathname;
    while (*mov != '\0') {
	if ((*mov) == '/') {
	    current = mov + 1;
	}
	mov++;
    }
    if (current && current-1 != pathname) {
	*(current-1) = '\0';
	ret = 1;
    }
    return ret;
}


/* static char *path_get_tail(char *pathname) */
/* { */
/*     char *tail; */
/*     tail = strtok(pathname, "/"); */
/*     while ((tail = strtok(NULL, "/"))) {} */
/*     return tail; */
/* } */

static DirPath *dirpath_create(const char *dirpath)
{
    DirPath *dp = calloc(1, sizeof(DirPath));
    strncpy(dp->path, dirpath, MAX_PATHLEN);
    return dp;
}

static FilePath *filepath_create(const char *filepath)
{
    FilePath *fp = calloc(1, sizeof(FilePath));
    strncpy(fp->path, filepath, MAX_PATHLEN);
    return fp;
}

static void filepath_destroy(FilePath *fp)
{
    free(fp);
}
static void dirpath_destroy(DirPath *dp)
{
    for (uint8_t i=0; i<dp->num_dirs; i++) {
	dirpath_destroy(dp->dirs[i]);
    }
    for (uint8_t i=0; i<dp->num_files; i++) {
	filepath_destroy(dp->files[i]);
    }
    free(dp);
}

DirPath *dirpath_open(const char *dirpath)
{
    char buf[MAX_PATHLEN];
    DirPath *dp = dirpath_create(dirpath);
    DIR *dir = opendir(dirpath);
    if (!dir) {
	dirpath_destroy(dp);
	perror("Failed to open dir");
	return NULL;
    }
    struct dirent *t_dir;
    while ((t_dir = readdir(dir))) {
	if (t_dir->d_type == DT_DIR) {
	    snprintf(buf, sizeof(buf), "%s/%s", dirpath, t_dir->d_name);
	    dp->dirs[dp->num_dirs] = dirpath_create(buf);
	    dp->num_dirs++;
	} else if (t_dir->d_type == DT_REG) {
	    snprintf(buf, sizeof(buf), "%s/%s", dirpath, t_dir->d_name);
	    dp->files[dp->num_files] = filepath_create(buf);
	    dp->num_files++;
	}
    }
    closedir(dir);
    return dp;
}

DirPath *dir_down(DirPath *dp, uint8_t index)
{
    if (index > dp->num_dirs - 1) {
	return NULL;
    }
    DirPath *to_free = dp;
    dp = dirpath_open(dp->dirs[index]->path);
    if (dp) {
	free(to_free);
	return dp;
    }
}

DirPath *dir_up(DirPath *dp)
{
    DirPath *to_free = dp;
    char buf[MAX_PATHLEN];
    strncpy(buf, dp->path, sizeof(buf));
    if (path_updir_name(buf)) {
	fprintf(stdout, "Success updir name: %s\n", buf);
	dirpath_destroy(to_free);
	dp = dirpath_open(buf);
	return dp;
    }
    return NULL;
}


static TLinesItem *create_tlitem_from_dir(void *dir_v)
{


}

DirNav *dirnav_create(const char *dir_name, Layout *lt, bool show_dirs, bool show_files)
{
    DirPath *dp = dirpath_open(dir_name);
    if (!dp) {
	return NULL;
    }
    DirNav *dn = calloc(1, sizeof(DirNav));
    dn->layout = lt;
    dn->show_dirs = show_dirs;
    dn->show_files = show_files;
    if (show_dirs) {
	for (uint8_t i=0; i<dp->num_dirs; i++) {
	    strcat(dn->line_text, dp->dirs[i]->path);
	    strcat(dn->line_text, "\n");
	}
    }
    if (show_files) {
	for (uint8_t i=0; i<dp->num_files; i++) {
	    strcat(dn->line_text, dp->files[i]->path);
	    strcat(dn->line_text, "\n");
	}
    }
    /* dn->lines = txt_area_create(dn->line_text, dn->layout, main_win->std_font, 14, color_global_white, main_win); */
    /* dn->lines->layout->children[0]->iterator->scrollable = true; */
    return dn;
}

void dirnav_draw(DirNav *dn)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black));
    SDL_RenderFillRect(main_win->rend, &dn->layout->rect);
    txt_area_draw(dn->lines);

}
/* void print_dir_contents(const char *dir_name, int indent) { */
/*     if (indent > 3) { */
/*         return; */
/*     } */

/*     DIR *dir = opendir(dir_name); */
/*     if (!dir) { */
/*         perror("opendir"); */
/*         return; */
/*     } */

/*     struct dirent *tdir; */
/*     while ((tdir = readdir(dir))) { */
/*         fprintf(stdout, "%*sName: %s\n", indent * 2, "", tdir->d_name); */

/*         if (tdir->d_type == DT_DIR && strcmp(tdir->d_name, ".") != 0 && strcmp(tdir->d_name, "..") != 0 && strncmp(tdir->d_name, ".", 1) != 0) { */
/*             char path[1024]; */
/*             snprintf(path, sizeof(path), "%s/%s", dir_name, tdir->d_name); */
/*             print_dir_contents(path, indent + 1); */
/*         } */
/*     } */

/*     closedir(dir); */
/* } */


void dir_tests()
{
    DirPath *dp = dirpath_open("/Users/charlievolow/Documents/c/jackdaw/gui/src");
    int i=0;
    DirPath *last = NULL;
    while (i<6 && (dp = dir_up(dp))) {
	last = dp;
	i++;
    }
    while (i<10 && (dp = dir_down(dp, 2))) {
	fprintf(stdout, "Down: %s\n", dp->path);
	last = dp;
	i++;
    }
    dp = last;
    if (dp) {
	for (uint8_t i=0; i<dp->num_dirs; i++) {
	    fprintf(stdout, "Dir: %s\n", dp->dirs[i]->path);
	}
	for (uint8_t i=0; i<dp->num_files; i++) {
	    fprintf(stdout, "File: %s\n", dp->files[i]->path);
	}
    }
}
