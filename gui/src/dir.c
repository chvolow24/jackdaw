#include "dir.h"
#include "textbox.h"


#define DIRNAV_LINE_SPACING 3
#define DIRNAV_TLINES_HEIGHT 200
#define SCROLL_TRIGGER_PAD (25 * main_win->dpi_scale_factor)
#define SCROLL_PAD (50 * main_win->dpi_scale_factor)

char SAVED_PROJ_DIRPATH[MAX_PATHLEN];

extern Window *main_win;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_clear;

SDL_Color color_dir = (SDL_Color) {10, 255, 100, 255};
SDL_Color color_file = (SDL_Color) {10, 100, 255, 255};
SDL_Color color_dir_unavailable = (SDL_Color) {10, 255, 100, 150};
SDL_Color color_file_unavailable = (SDL_Color) {10, 100, 255, 150};
SDL_Color color_dir_selected = (SDL_Color) {100, 255, 190, 255};
SDL_Color color_file_selected = (SDL_Color) {60, 150, 255, 255};
SDL_Color color_highlighted_bckgrnd = (SDL_Color) {255, 255, 255, 20};

int errno_saved = 0;
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


int path_updir_name(char *pathname)
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
    if (current) {
	if (current-1 != pathname) {
	    *(current-1) = '\0';
	    ret = 1;
	} else {
	    *(current) = '\0';
	    ret = 1;
	}
    }
    return ret;
}


char *path_get_tail(char *pathname)
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
    /* fprintf(stdout, "CREATING %p w %s\n", dp, dirpath); */
    return dp;
}

/* static FilePath *filepath_create(const char *filepath) */
/* { */
/*     FilePath *fp = calloc(1, sizeof(FilePath)); */
/*     strncpy(fp->path, filepath, MAX_PATHLEN); */
/*     return fp; */
/* } */

/* static void filepath_destroy(FilePath *fp) */
/* { */
/*     free(fp); */
/* } */
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
	errno_saved = errno;
	perror("Failed to open dir");
	return NULL;
    }
    struct dirent *t_dir;
    while ((t_dir = readdir(dir))) {
	if (strlen(dirpath) == 1 && dirpath[0] == '/') {
	    snprintf(buf, sizeof(buf), "/%s", t_dir->d_name);
	} else {
	    snprintf(buf, sizeof(buf), "%s/%s", dirpath, t_dir->d_name);
	}
	DirPath *subdir = dirpath_create(buf);
	if (subdir) {
	    char *tail = path_get_tail((char *)subdir->path);
	    if (strncmp(tail, ".", 1) == 0 && strncmp(tail, "..", 2) !=0)  {
		subdir->hidden = true;
	    }
	    dp->entries[dp->num_entries] = subdir;
	    subdir->type = t_dir->d_type;
	    dp->num_entries++;
	}
    }
    closedir(dir);
    return dp;
}

/* DirPath *dir_down(DirPath *dp, uint8_t index) */
/* { */
/*     if (index > dp->num_entries - 1) { */
/* 	return NULL; */
/*     } */
/*     DirPath *to_free = dp; */
/*     dp = dirpath_open(dp->entries[index]->path); */
/*     if (dp) { */
/* 	free(to_free); */
/* 	return dp; */
/*     } */
/*     return NULL; */
/* } */


DirPath *dir_up(DirPath *dp)
{
    DirPath *to_free = dp;
    char buf[MAX_PATHLEN];
    strncpy(buf, dp->path, sizeof(buf));
    if (path_updir_name(buf)) {
	/* fprintf(stdout, "Success updir name: %s\n", buf); */
	dirpath_destroy(to_free);
	DirPath *new = dirpath_open(buf);
	return new;
	/* if (new) { */
	/*     return new; */
	/* } else { */
	/*     return dp; */
	/* } */
    }
    return NULL;
}
DirPath *dir_select(DirPath *dp, DirPath *new)
{
    if (strncmp(path_get_tail(new->path), "..", 2) == 0) {
	new = dir_up(dp);
	/* if (!new) { */
	/*     return NULL; */
	/* } */

    } else {
	new = dirpath_open(new->path);
	if (new) {
	    dirpath_destroy(dp);
	}
    }
    /* if (!new) { */
    /* 	return dp; */
    /* } */

    return new;
}

static int qsort_dirnav_cmp(const void *tl1_v, const void*tl2_v)
{
    TLinesItem *tl1 = *((TLinesItem **)tl1_v);
    TLinesItem *tl2 = *((TLinesItem **)tl2_v);
    return strcmp(((DirPath *)tl1->obj)->path, ((DirPath *)tl2->obj)->path);
}

void sort_dn_lines(DirNav *dn)
{
    qsort(dn->lines->items, dn->lines->num_items, sizeof(TLinesItem *), qsort_dirnav_cmp);
    Layout *lines_container = dn->lines->container;
    TLinesItem *tli = NULL;
    for (uint16_t i=0; i<dn->lines->num_items; i++) {
	tli = dn->lines->items[i];
	tli->tb->layout = lines_container->children[i];
	tli->tb->text->container = tli->tb->layout;
	tli->tb->text->text_lt = tli->tb->layout->children[0];
	textbox_reset_full(tli->tb);
    }
}


/* int i=0; */
static TLinesItem *dir_to_tline(void ***current_item_v, Layout *container, void *dn_v, int (*filter)(void *item, void *x_arg))
{
    /* fprintf(stdout, "Call %d to dir_to_tline\n", i); */
    /* i++; */
    DirNav *dn = (DirNav *)dn_v;
    DirPath ***dps_loc = (DirPath ***)current_item_v;
    DirPath **dps = *dps_loc;
    DirPath *dp = dps[0];
    int filter_val;
    if (!dp) {
	fprintf(stderr, "ERROR: null at addr %p\n",*current_item_v);
	(*dps_loc)++;
	return NULL;	
    }
    if ((filter_val = filter(dp, dn_v)) == 0) {
	/* fprintf(stdout, "Item did not match filter\n"); */
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
    textbox_set_align(item->tb, CENTER_LEFT);
    textbox_size_to_fit(item->tb, 0, 0);
    item->tb->layout->w.value.floatval = 1.0;
    item->tb->layout->w.type = SCALE;
    textbox_reset_full(item->tb);
    SDL_Color *txt_clr =
	filter_val == -1 ?
	dp->type == DT_DIR ? &color_dir_unavailable : &color_file_unavailable :
	dp->type == DT_DIR ? &color_dir : &color_file;
    textbox_set_text_color(item->tb, txt_clr);
    textbox_set_background_color(item->tb, &color_global_clear);
    dn->num_lines++;
    /* dn->lines->num_items++; */
    (*dps_loc)++;
    return item;
}

/* static bool dir_to_tline_filter(void *item, void *x_arg) */
/* { */
/*     DirPath *dp = (DirPath *)item; */
/*     DirNav *dn = (DirNav *)x_arg; */
/*     if (!dn->show_dirs && dp->type == DT_DIR) return false; */
/*     if (!dn->show_files && dp->type != DT_DIR) return false; */
/*     if (dp->hidden) return false; */
/*     if (*(path_get_tail(dp->path)) - '.' < 0) return false; */

/*     return true; */
/* } */

void layout_write(FILE *f, Layout *lt, int indent);
DirNav *dirnav_create(const char *dir_name, Layout *lt, int (*dir_to_tline_filter)(void *item, void *x_arg))
{
    DirPath *dp = dirpath_open(dir_name);
    if (!dp) {
	return NULL;
    }
    /* fprintf(stdout, "Dirpath open\n"); */
    /* fprintf(stdout, "Dirpath num items? %d\n", dp->num_entries); */
    /* for (uint16_t i=0; i<dp->num_entries; i++) { */
    /* 	fprintf(stdout, "Entry %d/%d: %s\n", i, dp->num_entries, dp->entries[i]->path); */
    /* } */
    /* exit(0); */

    DirNav *dn = calloc(1, sizeof(DirNav));
    dn->dirpath = dp;
    dn->dir_to_tline_filter = dir_to_tline_filter;
    dn->layout = lt;
    strcpy(lt->name, "dirnav_lt");
    Layout *inner = layout_add_child(lt);
    strcpy(inner->name, "dirnav_lines_container");
    inner->x.value.intval = 5;
    inner->y.value.intval = 5;
    inner->w.type = PAD;
    inner->h.type = ABS;
    inner->h.value.intval = DIRNAV_TLINES_HEIGHT;
    /* dn->show_dirs = show_dirs; */
    /* dn->show_files = show_files; */
    lt->h.value.intval = DIRNAV_TLINES_HEIGHT;
    layout_reset(lt);
    Layout *lines_container = layout_add_child(inner);
    lines_container->w.value.floatval = 1.0;
    lines_container->h.value.floatval = 1.0;
    lines_container->w.type = SCALE;
    lines_container->h.type = SCALE;
    dn->lines = textlines_create((void **)dn->dirpath->entries, dp->num_entries, dir_to_tline_filter, dir_to_tline, lines_container, (void *)dn);
    dn->lines->num_items = dn->num_lines;
    sort_dn_lines(dn);
    /* qsort(dn->lines->items, dn->lines->num_items, sizeof(TLinesItem *), qsort_dirnav_cmp); */


    TLinesItem *sel = dn->lines->items[dn->current_line];
    SDL_Color sel_clr = (((DirPath *)sel->obj)->type == DT_DIR) ? color_dir_selected : color_file_selected;
    /* sel_clr = (SDL_Color) {255, 255, 255, 255}; */
    textbox_set_text_color(dn->lines->items[dn->current_line]->tb, &sel_clr);
    textbox_set_background_color(sel->tb, &color_highlighted_bckgrnd);
    textbox_reset_full(sel->tb);

    Layout *pathname_lt = layout_add_child(dn->layout);
    dn->current_path_tb = textbox_create_from_str((char *)dir_name, pathname_lt, main_win->bold_font, 12, main_win);
    textbox_set_align(dn->current_path_tb, CENTER_LEFT);
    textbox_set_background_color(dn->current_path_tb, &color_global_clear);
    textbox_set_text_color(dn->current_path_tb, &color_global_white);
    textbox_size_to_fit_v(dn->current_path_tb, 0);
    /* lt->h.value.intval += pathname_lt->h.value.intval; */
    pathname_lt->x.value.intval = 5;
    /* pathname_lt->rect.y = dn->layout->rect.y + dn->layout->rect.y - inner->rect.h - inner->rect.y; */
    pathname_lt->y.type = STACK;
    pathname_lt->y.value.intval = 5;
    pathname_lt->w.type = PAD;
    /* pathname_lt->rect.h = dn->layout->rect.h - inner->rect.h - inner->rect.y; */
    pathname_lt->w.value.intval = DIRNAV_LINE_SPACING;
    /* laoyut */
    /* pathname_lt->rect.y = dn->layout->rect.y + dn->layout->rect.h - pathname_lt->rect.h; */
    /* layout_set_values_from_rect(pathname_lt); */
    textbox_reset_full(dn->current_path_tb);
    layout_size_to_fit_children(dn->layout, true, 30);
    layout_reset(dn->layout);
    /* layout_write(stdout, pathname_lt, 0); */
    /* layout_reset(dn->layout); */
    return dn;
}

void dirnav_destroy(DirNav *dn)
{
    dirpath_destroy(dn->dirpath);
    if (dn->instruction) {
	textbox_destroy(dn->instruction);
    }
    if(dn->current_path_tb) {
	textbox_destroy(dn->current_path_tb);
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
    /* layout_reset(dn->layout); */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(control_bar_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &dn->layout->rect);
    /* SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255); */
    /* SDL_RenderDrawRect(main_win->rend, &dn->lines->container->rect); */
    Layout *inner = layout_get_child_by_name_recursive(dn->layout, "dirnav_lines_container");
    SDL_RenderSetClipRect(main_win->rend, &inner->rect);
    for (uint8_t i=0; i<dn->lines->num_items; i++) {
	TLinesItem *tli = NULL;
	/* fprintf(stdout, "Dn draw %s, %s\n", dn->lines->items[i]->tb->text->value_handle, dn->lines->items[i]->tb->text->display_value); */
	if ((tli = dn->lines->items[i]) == NULL || tli->tb == NULL) {
	    continue;
	}
	textbox_reset_full(tli->tb);
	textbox_draw(tli->tb);
    }
    SDL_RenderSetClipRect(main_win->rend, &main_win->layout->rect);
    if (dn->current_path_tb) {
	textbox_draw(dn->current_path_tb);
    }
    /* SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
    /* SDL_RenderDrawRect(main_win->rend, &dn->lines->container->rect); */
    /* SDL_RenderDrawRect(main_win->rend, &dn->layout->rect); */
    /* SDL_RenderDrawRect(main_win->rend, &inner->rect); */
    
    /* layout_draw(main_win, dn->layout); */
    /* layout_draw(main_win, dn->lines->container); */
}

/* void layout_write(FILE *f, Layout *lt, int indent); */


TLinesItem *dirnav_select_item(DirNav *dn, uint16_t i)
{
    if (i<dn->num_lines && i>= 0) {
	TLinesItem *current = dn->lines->items[dn->current_line];
	textbox_set_background_color(current->tb, &color_global_clear);
	textbox_reset_full(current->tb);
	dn->current_line = i;
	current = dn->lines->items[dn->current_line];
	textbox_set_background_color(current->tb, &color_highlighted_bckgrnd);
	layout_reset(dn->layout);
	return current;

	/* Layout *inner = layout_get_child_by_name_recursive(dn->layout, "dirnav_lines_container"); */
	/* if (current->tb->layout->rect.y + current->tb->layout->rect.h > inner->rect.y + inner->rect.h - SCROLL_TRIGGER_PAD) { */
	/*     int item_abs_y = current->tb->layout->rect.y; */
	/*     int item_desired_y = inner->rect.y + inner->rect.h - SCROLL_PAD; */
	/*     dn->lines->container->scroll_offset_v -= item_abs_y - item_desired_y; */
	/* } */
    }
    return NULL;

}

void dirnav_next(DirNav *dn)
{
    TLinesItem *current = dirnav_select_item(dn, dn->current_line + 1);
    if (current) {
	Layout *inner = layout_get_child_by_name_recursive(dn->layout, "dirnav_lines_container");
	/* TLinesItem *current = dn->lines-> */
	if (current->tb->layout->rect.y + current->tb->layout->rect.h > inner->rect.y + inner->rect.h - SCROLL_TRIGGER_PAD) {
	    int item_abs_y = current->tb->layout->rect.y;
	    int item_desired_y = inner->rect.y + inner->rect.h - SCROLL_PAD;
	    dn->lines->container->scroll_offset_v -= item_abs_y - item_desired_y;
	}
    }
}

void dirnav_previous(DirNav *dn)
{
    TLinesItem *current = dirnav_select_item(dn, dn->current_line - 1);
    if (current) {
	Layout *inner = layout_get_child_by_name_recursive(dn->layout, "dirnav_lines_container");
	if (current->tb->layout->rect.y < inner->rect.y + SCROLL_TRIGGER_PAD && dn->lines->container->scroll_offset_v < 0) {
	    int item_abs_y = current->tb->layout->rect.y;
	    int item_desired_y = inner->rect.y + SCROLL_PAD;
	    dn->lines->container->scroll_offset_v -= item_abs_y - item_desired_y;
	    if (dn->lines->container->scroll_offset_v > 0) dn->lines->container->scroll_offset_v = 0;
	    layout_reset(dn->lines->container);
	}
    }
}

void dirnav_select(DirNav *dn)
{
    DirPath *selected = (DirPath *)dn->lines->items[dn->current_line]->obj;
    if (selected->type == DT_DIR) {
	DirPath *new = dir_select(dn->dirpath, selected);
	if (!new) {
	    textbox_set_value_handle(dn->current_path_tb, strerror(errno_saved));
	    return;
	}
	dn->dirpath = new;
	Layout *lines_container = dn->lines->container;
	textlines_destroy(dn->lines);
	dn->num_lines = 0;
	dn->current_line = 0;
	/* for (uint8_t i = 0; i<dn->layout->num_children; i++) { */
	/*     layout_destroy(dn->layout->children[i]); */
	/* } */
	/* Layout *inner = layout_get_child_by_name_recursive(dn->layout, "dirnav_lines_container"); */
	for (uint8_t i=0; i<lines_container->num_children; i++) {
	    layout_destroy(lines_container->children[i]);
	}
	/* Layout *inner = layout_add_child(dn->layout); */
	/* inner->x.value.intval = 5; */
	/* inner->y.value.intval = 5; */
	/* inner->w.type = PAD; */
	/* inner->h.type = PAD; */
	lines_container->scroll_offset_v = 0;
	/* layout_reset(lines_container); */

	/* Layout *lines_container = dn->layout->children[0]; */
	/* for (uint8_t i=0; i<lines_container->num_children; i++) { */
	/* 	fprintf(stdout, "destroy!\n"); */
	/* 	layout_destroy(lines_container->children[i]); */
	/* } */
	dn->lines = textlines_create((void **)dn->dirpath->entries, dn->dirpath->num_entries, dn->dir_to_tline_filter, dir_to_tline, lines_container, (void *)dn);
	sort_dn_lines(dn);
	TLinesItem *sel = dn->lines->items[dn->current_line];
	SDL_Color sel_clr = (((DirPath *)sel->obj)->type == DT_DIR) ? color_dir_selected : color_file_selected;
	/* sel_clr = (SDL_Color) {255, 255, 255, 255}; */
	textbox_set_text_color(dn->lines->items[dn->current_line]->tb, &sel_clr);
	textbox_set_background_color(sel->tb, &color_highlighted_bckgrnd);
	textbox_reset_full(sel->tb);
	textbox_set_value_handle(dn->current_path_tb, new->path);
	layout_reset(dn->layout);
    } else if (selected->type == DT_REG && dn->file_select_action) {
	dn->file_select_action(dn, selected);
    }
}

void dirnav_triage_click(DirNav *dn, SDL_Point *mousep)
{
    for (uint16_t i=0; i<dn->num_lines; i++) {
	TLinesItem *line = dn->lines->items[i];
	if (SDL_PointInRect(mousep, &line->tb->layout->rect)) {
	    dn->current_line = i;
	    dirnav_select(dn);
	    break;
	}
	
    }
}

void dir_tests()
{
    /* DirPath *dp = dirpath_open("/Users/charlievolow/Documents/c/jackdaw/gui/src"); */
    /* int i=0; */
    /* DirPath *last = NULL; */
    /* while (i<4 && (dp = dir_up(dp))) { */
    /* 	last = dp; */
    /* 	i++; */
    /* } */
    /* while (i<4 && (dp = dir_down(dp, 2))) { */
    /* 	fprintf(stdout, "Down: %s\n", dp->path); */
    /* 	last = dp; */
    /* 	i++; */
    /* } */
    /* dp = last; */
    /* fprintf(stdout, "DP num entries: %d\n", dp->num_entries); */
    /* if (dp) { */
    /* 	for (uint8_t i=0; i<dp->num_entries; i++) { */
    /* 	    fprintf(stdout, "Dir: %s, type: %s\n", dp->entries[i]->path, filetype(dp->entries[i]->type)); */
    /* 	} */
    /* } */
}
