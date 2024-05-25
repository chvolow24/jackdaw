#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir.h"
#include "textbox.h"


#define DIRNAV_LINE_SPACING 3
#define DIRNAV_HEIGHT 200
extern Window *main_win;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_clear;

SDL_Color color_dir = (SDL_Color) {10, 255, 100, 255};
SDL_Color color_file = (SDL_Color) {10, 100, 255, 255};

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


static char *path_get_tail(char *pathname)
{
    char *mov = pathname;
    char *slash = pathname;
    while (*mov != '\0') {
	if (*mov == '/') {
	    slash = mov + 1;
	}
	mov++;
    }
    return slash;
    /* char *tail; */
    /* tail = strtok(pathname, "/"); */
    /* while ((tail = strtok(NULL, "/"))) { */
    /* 	fprintf(stdout, "Tail: %s\n", tail);} */
    /* fprintf(stdout, "Tail out of loop: %s\n", tail); */
    /* return tail; */
}

static DirPath *dirpath_create(const char *dirpath)
{
    DirPath *dp = calloc(1, sizeof(DirPath));
    strncpy(dp->path, dirpath, MAX_PATHLEN);
    fprintf(stdout, "CREATING %p w %s\n", dp, dirpath);
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
    for (uint8_t i=0; i<dp->num_entries; i++) {
	dirpath_destroy(dp->entries[i]);
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
	snprintf(buf, sizeof(buf), "%s/%s", dirpath, t_dir->d_name);
	DirPath *subdir = dirpath_create(buf);
	if (subdir) {
	    dp->entries[dp->num_entries] = subdir;
	    subdir->type = t_dir->d_type;
	    dp->num_entries++;
	}
    }
    closedir(dir);
    return dp;
}

DirPath *dir_down(DirPath *dp, uint8_t index)
{
    if (index > dp->num_entries - 1) {
	return NULL;
    }
    DirPath *to_free = dp;
    dp = dirpath_open(dp->entries[index]->path);
    if (dp) {
	free(to_free);
	return dp;
    }
    return NULL;
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

int i=0;
static TLinesItem *dir_to_tline(void ***current_item_v, Layout *container, void *dn_v, bool (*filter)(void *item, void *x_arg))
{
    fprintf(stdout, "Call %d to dir_to_tline\n", i);
    i++;
    DirNav *dn = (DirNav *)dn_v;
    DirPath ***dps_loc = (DirPath ***)current_item_v;
    DirPath **dps = *dps_loc;
    DirPath *dp = dps[0];
    fprintf(stdout, "\tDp: %p\n", dp);
    if (!dp) {
	fprintf(stderr, "ERROR: null at addr %p\n",*current_item_v);
	(*dps_loc)++;
	return NULL;	
    }
    if (!filter(dp, dn_v)) {
	fprintf(stdout, "Item did not match filter\n");
	(*dps_loc)++;
	return NULL;
    }

    /* if ((!dn->show_dirs && dp->type == DT_DIR) || (!dn->show_files && dp->type != DT_DIR)) { */
    /* 	/\* dn->lines->num_items++; *\/ */
    /* 	fprintf(stdout, "Skipping\n"); */
    /* 	/\* dn->num_lines++; *\/ */
    /* 	(*dps_loc)++; */
    /* 	return NULL; */
    /* } */
    TLinesItem *item = calloc(1, sizeof(TLinesItem));
    item->obj = (void *)dp;
    Layout *lt = layout_add_child(container);
    lt->y.type = STACK;
    lt->y.value.intval = DIRNAV_LINE_SPACING;
    lt->h.value.intval = 50;
    lt->w.value.intval = 500;
    
    item->tb = textbox_create_from_str(path_get_tail(dp->path), lt, main_win->bold_font, 12, main_win);
    /* textbox_set_pad(item->tb, 0, 4); */
    textbox_size_to_fit(item->tb, 0, 0);
    textbox_reset_full(item->tb);
    SDL_Color *txt_clr = dp->type == DT_DIR ? &color_dir : &color_file;
    textbox_set_text_color(item->tb, txt_clr);
    textbox_set_background_color(item->tb, &color_global_clear);
    dn->num_lines++;
    /* dn->lines->num_items++; */
    (*dps_loc)++;
    return item;
}

bool dir_to_tline_filter(void *item, void *x_arg)
{
    DirPath *dp = (DirPath *)item;
    DirNav *dn = (DirNav *)x_arg;
    if (!dn->show_dirs && dp->type == DT_DIR) {
	return false;
    }
    if (!dn->show_files && dp->type != DT_DIR) {
	return false;
    }
    return true;
}

DirNav *dirnav_create(const char *dir_name, Layout *lt, bool show_dirs, bool show_files)
{
    DirPath *dp = dirpath_open(dir_name);
    if (!dp) {
	return NULL;
    }
    fprintf(stdout, "Dirpath open\n");
    fprintf(stdout, "Dirpath num items? %d\n", dp->num_entries);
    for (uint16_t i=0; i<dp->num_entries; i++) {
	fprintf(stdout, "Entry %d/%d: %s\n", i, dp->num_entries, dp->entries[i]->path);
    }
    /* exit(0); */

    DirNav *dn = calloc(1, sizeof(DirNav));
    dn->dirpath = dp;
    dn->layout = lt;
    Layout *inner = layout_add_child(lt);
    inner->x.value.intval = 5;
    inner->y.value.intval = 4;
    inner->w.type = PAD;
    inner->h.type = PAD;
    dn->show_dirs = show_dirs;
    dn->show_files = show_files;
    lt->h.value.intval = DIRNAV_HEIGHT;
    layout_reset(lt);
    dn->lines = textlines_create((void **)dn->dirpath->entries, dp->num_entries, dir_to_tline_filter, dir_to_tline, inner, (void *)dn);
    dn->lines->num_items = dn->num_lines;
    return dn;
}

void dirnav_destroy(DirNav *dn)
{
    dirpath_destroy(dn->dirpath);
    if (dn->instruction) {
	textbox_destroy(dn->instruction);
    }
    if(dn->current_path) {
	textbox_destroy(dn->current_path);
    }
    if (dn->lines) {
	textlines_destroy(dn->lines);
    }
    if (dn->layout) {
	layout_destroy(dn->layout);
    }
    free(dn);
}

extern SDL_Color control_bar_bckgrnd;
void dirnav_draw(DirNav *dn)
{

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(control_bar_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &dn->layout->rect);
    SDL_RenderSetClipRect(main_win->rend, &dn->lines->container->rect);
    for (uint8_t i=0; i<dn->lines->num_items; i++) {
	TLinesItem *tli = NULL;
	/* fprintf(stdout, "Dn draw %s, %s\n", dn->lines->items[i]->tb->text->value_handle, dn->lines->items[i]->tb->text->display_value); */
	if ((tli = dn->lines->items[i]) == NULL || tli->tb == NULL) {
	    continue;
	}
	textbox_reset_full(dn->lines->items[i]->tb);
	textbox_draw(dn->lines->items[i]->tb);
    }
    SDL_RenderSetClipRect(main_win->rend, &main_win->layout->rect);
/*     SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black)); */
/*     SDL_RenderFillRect(main_win->rend, &dn->layout->rect); */
/*     txt_area_draw(dn->lines); */

/* } */
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
}


void dir_tests()
{
    DirPath *dp = dirpath_open("/Users/charlievolow/Documents/c/jackdaw/gui/src");
    int i=0;
    DirPath *last = NULL;
    while (i<4 && (dp = dir_up(dp))) {
	last = dp;
	i++;
    }
    while (i<4 && (dp = dir_down(dp, 2))) {
	fprintf(stdout, "Down: %s\n", dp->path);
	last = dp;
	i++;
    }
    dp = last;
    fprintf(stdout, "DP num entries: %d\n", dp->num_entries);
    if (dp) {
	for (uint8_t i=0; i<dp->num_entries; i++) {
	    fprintf(stdout, "Dir: %s, type: %s\n", dp->entries[i]->path, filetype(dp->entries[i]->type));
	}
    }
}
