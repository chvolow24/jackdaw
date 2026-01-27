#include "log.h"
#include "thread_safety.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static FILE *logfile[NUM_JDAW_THREADS] = {0};
static char logfile_path[NUM_JDAW_THREADS][1024];
/* pthread_mutex_t log_mutex; */


const char *log_level_str(enum log_level level)
{
    switch (level) {
    case LOG_FATAL:
	return "FATAL";
    case LOG_ERROR:
	return "ERROR";
    case LOG_WARN:
	return "WARN";
    case LOG_INFO:
	return "INFO";
    case LOG_DEBUG:
	return "DEBUG";
    }
    return NULL;
}

const char* system_tmp_dir() {
    static char temp_path[1024] = {0};

    /* check TMPDIR env var */
    char *env_tmp = getenv("TMPDIR");
    if (env_tmp && env_tmp[0] != '\0') {
	snprintf(temp_path, sizeof(temp_path), "%s", env_tmp);
        return temp_path;
    }

    /* else check /tmp */
    if (access("/tmp", W_OK) == 0) {
	snprintf(temp_path, sizeof(temp_path), "/tmp");
        strncpy(temp_path, "/tmp", sizeof(temp_path) - 1);
        return temp_path;
    }

    /* else use cwd */
    return ".";
}

void log_init()
{
    for (int i=0; i<NUM_JDAW_THREADS; i++) {
	snprintf(logfile_path[i], sizeof(logfile_path[i]), "%s/jackdaw_exec_%s.log", system_tmp_dir(), get_thread_name(i));
	logfile[i] = fopen(logfile_path[i], "w");
	
	if (!logfile[i]) {
	    fprintf(stderr, "Error: unable to open logfile at %s\n", logfile_path[i]);
	}
    }
    /* int err = pthread_mutex_init(&log_mutex, NULL); */
    /* if (err != 0) { */
    /* 	fprintf(stderr, "Error opening log mutex: %s\n", strerror(err)); */
    /* } */
}

void log_quit()
{
    for (int i=0; i<NUM_JDAW_THREADS; i++) {
	if (logfile[i]) {
	    fclose(logfile[i]);
	}
    }
}

const char *timestamp()
{
  struct timeval tv;
  struct tm *timeinfo;
  gettimeofday(&tv, NULL);
  timeinfo = localtime(&tv.tv_sec);
  /* enum jdaw_thread thread = current_thread(); */
  static JDAW_THREAD_LOCAL char buf[64] = {0};
  char *ms_loc = buf + strftime(buf, 64, "%Y-%m-%d %H:%M:%S", timeinfo);
  snprintf(ms_loc, sizeof(buf) - (ms_loc - buf), ".%06d", tv.tv_usec);
  return buf;
}

void log_tmp(enum log_level level, char *fmt, ...)
{
    #ifndef TESTBUILD
    if (level == LOG_DEBUG) return;
    #endif
    enum jdaw_thread thread = current_thread();
    if (logfile[thread]) {
	const char *timestamp_loc = timestamp();
	/* fprintf(stderr, "(%s) %s [%s]:f ", log_level_str(level), timestamp_loc, get_thread_name(thread)); */
	fprintf(logfile[thread], "(%s) %s [%s]: ", log_level_str(level), timestamp_loc, get_thread_name(thread));
	va_list ap;
	va_start(ap, fmt);
	vfprintf(logfile[thread], fmt, ap);
	/* va_start(ap, fmt); */
	/* vfprintf(stderr, fmt, ap); */
	fflush(logfile[thread]);
	va_end(ap);
    } else {
	fprintf(stderr, "Thread \"%s\" logfile not yet created or unset\n", get_thread_name(thread));
    }
    /* pthread_mutex_unlock(&log_mutex); */
}

void log_print_current_thread()
{
    /* pthread_mutex_lock(&log_mutex); */
    enum jdaw_thread thread = current_thread();
    fflush(logfile[thread]);
    FILE *logread = fopen(logfile_path[thread], "r");
    int c;
    while ((c = fgetc(logread)) != EOF) {
	fputc(c, stderr);
    }
}


static int line_cmp(const void *v1, const void *v2)
{
    /* 1994-04-25 10:03:21.2222 */
    const int ts_fmt_len = 25;
    const char *str1 = *(char **)v1;
    const char *str2 = *(char **)v2;
    while (*str1 != ')') str1++;
    str1++; str1++;
    while (*str2 != ')') str2++;
    str2++; str2++;
    return strncmp(str1, str2, ts_fmt_len);
}

/* Merges all thread logs and prints in order */
void log_printall()
{
    int lines_cap = 512;
    int num_lines = 0;
    char **lines = malloc(lines_cap * sizeof(char *));
    /* FILE *logfiles[NUM_JDAW_THREADS]; */
    /* bool more_lines[NUM_JDAW_THREADS]; */
    /* char *line_queued[NUM_JDAW_THREADS]; */

    for (int i=0; i<NUM_JDAW_THREADS; i++) {
	fflush(logfile[i]);
	FILE *readfile = fopen(logfile_path[i], "r");
	while (1) {
	    char *line = NULL;
	    size_t linecap = 0;
	    ssize_t linelen;

	    linelen = getline(&line, &linecap, readfile);
	    if (linelen <= 0) break;
	    lines[num_lines] = line;
	    num_lines++;
	    if (num_lines == lines_cap) {
		lines_cap *= 2;
		lines = realloc(lines, lines_cap * sizeof(char *));
	    }
	}
	fclose(readfile);
    }

    qsort(lines, num_lines, sizeof(char *), line_cmp);

    for (int i=0; i<num_lines; i++) {
	fprintf(stderr, "%s", lines[i]);
	free(lines[i]);
    }
    free(lines);

    fprintf(stderr, "... done printing %d lines\n", num_lines);    
}
