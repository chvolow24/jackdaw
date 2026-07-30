#include "log.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

const char* system_tmp_dir() {
    static char *ret = NULL;
    if (ret) return ret;
    static char temp_path[PATH_MAX] = {0};

    /* check TMPDIR env var */
    char *env_tmp = getenv("TMPDIR");
    if (env_tmp && env_tmp[0] != '\0') {
	snprintf(temp_path, sizeof(temp_path), "%s", env_tmp);
        ret = temp_path;
	return ret;
    }

    /* else check /tmp */
    if (access("/tmp/", W_OK) == 0) {
	snprintf(temp_path, sizeof(temp_path), "/tmp/");
        ret = temp_path;
	return ret;
    }

    /* else use cwd */
    ret = "./";
    return ret;
}

char *create_tmp_file(const char *name)
{
    char filepath[1024] = {0};
    int offset = snprintf(filepath, 1024, "%s", system_tmp_dir());
    if (filepath[offset - 1] == '/') {
	snprintf(filepath + offset, 1024 - offset, "%s", name);
    } else {
	snprintf(filepath + offset, 1024 - offset, "/%s", name);
    }
    return strdup(filepath);    
}

int safe_delete_tmp_file(const char *filepath)
{
    const char *tmp_dir = system_tmp_dir();
    if (strncmp(tmp_dir, filepath, strlen(tmp_dir)) == 0) {
        if (remove(filepath) != 0) {
            log_tmp(LOG_ERROR, "Error removing file at %s\n", filepath);
        }
        return 0;
    } else {
        log_tmp(LOG_ERROR, "Ignoring unsafe attempt to delete file at %s\n", filepath);
        return -1;
    }
}
