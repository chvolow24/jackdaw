#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define MAX_DIRS 255
#define MAX_FILES 255
#define MAX_PATHLEN 255

typedef struct dirpath DirPath;
typedef struct filepath FilePath;

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


char *dir_get_homepath()
{
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    return pw->pw_dir;
}

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
    if (current && current-1 != pathname) {
	*(current-1) = '\0';
	ret = 1;
    }
    return ret;
}


char *path_get_tail(char *pathname)
{
    char *tail;
    tail = strtok(pathname, "/");
    while ((tail = strtok(NULL, "/"))) {}
    return tail;
}

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
    /* if (dp) { */
    /* 	dirpath_destroy(to_free); */
    /* 	return dp; */
    /* } else { */
    /* 	return NULL; */
    /* } */
	
    /* for (uint8_t i=0; i<dp->num_dirs; i++) { */
    /* 	if (strncmp(path_get_tail(dp->dirs[i]->path), "..", 2) == 0) { */
    /* 	    dp = dirpath_open(dp->dirs[i]->path); */
    /* 	    dirpath_destroy(to_free); */
    /* 	    return dp; */
    /* 	} */
    /* } */
}

void print_dir_contents(const char *dir_name, int indent) {
    if (indent > 3) {
        return;
    }

    DIR *dir = opendir(dir_name);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *tdir;
    while ((tdir = readdir(dir))) {
        fprintf(stdout, "%*sName: %s\n", indent * 2, "", tdir->d_name);

        if (tdir->d_type == DT_DIR && strcmp(tdir->d_name, ".") != 0 && strcmp(tdir->d_name, "..") != 0 && strncmp(tdir->d_name, ".", 1) != 0) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir_name, tdir->d_name);
            print_dir_contents(path, indent + 1);
        }
    }

    closedir(dir);
}
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
    /* fprintf(stdout, "Home dir : %s\n", dir_get_homepath()); */
    /* print_dir_contents(INSTALL_DIR, 0); */
    
    /* DIR *homedir = opendir(dir_get_homepath()); */
    /* struct dirent *tdir; */
    /* while ((tdir = readdir(homedir))) { */
    /* 	fprintf(stdout, "Name: %s, type: %s\n", tdir->d_name, filetype(tdir->d_type)); */
    /* 	if (tdir->d_type == DT_DIR) { */
    /* 	    dir_tests() */
    /* 	} */
    /* } */

}
