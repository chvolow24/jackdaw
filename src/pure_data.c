#include <stdio.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "audio_connection.h"
#include "project.h"
#include "transport.h"
/* #include "pure_data.h" */

#define MAX_SEMNAME 255
#define MAX_BUFLEN 4096

#define PD_JACKDAW_SHM_PIDS_NAME "/pd_jackdaw_pids"
#define PD_JACKDAW_SHM_NAME "/pd_jackdaw_shm"
#define PD_AUDIOPARAMS_SEMNAME "/pd_jackdaw_audioparam_sem"
#define PD_AUDIOBUFFERS_WRITE_SEMNAME "/pd_jackdaw_audiobuf_write_sem"
#define PD_AUDIOBUFFERS_READ_SEMNAME "/pd_jackdaw_audiobuf_read_sem"

struct pd_jackdaw_shm_pids {
    pid_t jackdaw_pid;
    pid_t pd_pid;
};

pid_t pd_pid;

typedef struct pd_input {
    char secret[255];
    uint8_t secret_len;
    uint16_t pd_blocksize;
    bool recording;
    float L[MAX_BUFLEN];
    float R[MAX_BUFLEN];
} PdInput;


struct pd_jackdaw_shm_pids *pids = NULL;
PdInput *pd_shm = NULL;
bool handshake_done = false;
bool connection_open = false;
uint16_t pd_blocksize = 0;
sem_t *audio_params_sem = NULL;
sem_t *audio_buffers_write_sem = NULL;
sem_t *audio_buffers_read_sem = NULL;

#if defined(__linux__)
static int validate_pid(pid_t pid, const char *expected_cmdline) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char cmdline[256];
    if (fgets(cmdline, sizeof(cmdline), f) == NULL) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return (strstr(cmdline, expected_cmdline) != NULL);
}
#elif defined(__APPLE__) && defined(__MACH__)
#include <libproc.h>
static int validate_pid(pid_t pid, const char *expected_cmdline) {
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, pathbuf, sizeof(pathbuf)) <= 0) {
        return 0;
    }

    // Compare the path with the expected command line string
    return (strstr(pathbuf, expected_cmdline) != NULL);
}
#else
#error "Unsupported platform"
#endif


/* Once confirmed that both processes are active, create shared memory for other data */
static void complete_handshake()
{
    fprintf(stdout, "Completing handshake\n");
    /* To complete handshake:
       1. respond to pd to indicate completion
       2. create main shm
       3. send another SIGUSR1 to indicate main shm complete
       4. destroy PID shm
    */
    
    kill(pids->pd_pid, SIGUSR2);

    shm_unlink(PD_JACKDAW_SHM_NAME); /* Don't care about errors */

    int fd = shm_open(PD_JACKDAW_SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
	perror("Error opening main shm (shm_open)");
    } else if (ftruncate(fd, sizeof(PdInput)) == -1) {
	perror("Error opening main shm (ftruncate)");
    }

    pd_shm = (PdInput *)mmap(NULL, sizeof(PdInput), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pd_shm == MAP_FAILED) {
	perror("Error mapping main shm");
    }
    memset(pd_shm, '\0', sizeof(PdInput));

    const char *secret = "I am a silly goose";
    strcpy(pd_shm->secret, secret);
    pd_shm->secret_len = strlen(secret);

    /* Initialize semaphores */
    if (sem_unlink(PD_AUDIOPARAMS_SEMNAME) != 0) {
	perror("Unable to unlink named semaphore");
    }
    if (sem_unlink(PD_AUDIOBUFFERS_READ_SEMNAME) != 0) {
	perror("Unable to unlink named semaphore");
    }
    if (sem_unlink(PD_AUDIOBUFFERS_WRITE_SEMNAME) != 0) {
	perror("Unable to unlink named semaphore");
    }

    audio_params_sem = sem_open(PD_AUDIOPARAMS_SEMNAME, O_CREAT | O_EXCL, 0666, 0);
    if (audio_params_sem == SEM_FAILED) {
	perror("Failed to open params semaphore");
    }
    audio_buffers_read_sem = sem_open(PD_AUDIOBUFFERS_READ_SEMNAME, O_CREAT | O_EXCL, 0666, 0);
    if (audio_buffers_read_sem == SEM_FAILED) {
	perror("Failed to open buffer semapahore");
    }
    audio_buffers_write_sem = sem_open(PD_AUDIOBUFFERS_WRITE_SEMNAME, O_CREAT | O_EXCL, 0666, 0);
    if (audio_buffers_write_sem == SEM_FAILED) {
	perror("Failed to open buffer semapahore");
    }

    
    /* Indicate that shm and sems can be safely opened */
    kill(pids->pd_pid, SIGUSR1);
    fprintf(stdout, "Sent signal 1 to pd; pd can now open main shm and sems\n");

    /* Unlink PIDS shared memory object; no more signals will be passed. */
    pd_pid = pids->pd_pid;
    /* shm_unlink(PD_JACKDAW_SHM_PIDS_NAME); */
    connection_open = true;
}

int pd_jackdaw_shm_init();
static void handle_signal1_from_pd(int signo)
{
    if (connection_open) {
	fprintf(stdout, "Pure data seems to have closed. Closing connection.\n");
	connection_open = false;
	handshake_done = false;
	pd_jackdaw_shm_init();
    } else {
	fprintf(stdout, "Completing handshake. jackdaw started first and received sig1 from pd\n");
	complete_handshake();
    }
}

static void handle_signal2_from_pd(int signo)
{
    fprintf(stdout, "Completing handshake. pd started first, responded to my message\n");
    complete_handshake();
}

static void initiate_handshake()
{
     if (pids->pd_pid != 0 && validate_pid(pids->pd_pid, "pd")) {
	fprintf(stdout, "Identified pd PID and validated. Sending signal...\n");
	kill(pids->pd_pid, SIGUSR1);
     } else {
	 fprintf(stderr, "Could not validate pd PID\n");
     }
}

void pd_signal_termination_of_jackdaw()
{
    if (pd_pid != 0) {
	kill(pd_pid, SIGUSR1);
    }
}

int pd_jackdaw_shm_init()
{
    /* if (!restart) { */
    signal(SIGUSR1, handle_signal1_from_pd);
    signal(SIGUSR2, handle_signal2_from_pd);
    /* } */
    int fd = shm_open(PD_JACKDAW_SHM_PIDS_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
	perror("Error opening pids shm (shm_open)");
    } else if (ftruncate(fd, sizeof(struct pd_jackdaw_shm_pids)) == -1) {
	perror("Error opening pids shm (ftruncate)");
    }

    pids = (struct pd_jackdaw_shm_pids *)mmap(NULL, sizeof(struct pd_jackdaw_shm_pids), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    pids->jackdaw_pid = getpid();

    initiate_handshake();
    return 0;


    
    /* /\* int fd = shm_open(PD_JACKDAW_SHM_NAME, O_CREAT | O_RDWR, 0666); *\/ */
    /* if (fd == -1) { */
    /* 	perror("Error in shm open"); */
    /* } else if (ftruncate(fd, sizeof(PdInput)) == -1) { */
    /* 	perror("Error in ftruncate"); */
    /* } */

    /* void *mem = mmap(NULL, sizeof(PdInput), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); */
    
    /* if (mem == MAP_FAILED) { */
    /* 	perror("Error in mmap\n"); */
    /* } */
    /* close(fd); */

    /* pd_shm = (PdInput *)mem; */

    /* pd_shm->jackdaw_pid = getpid(); */
    /* fprintf(stdout, "Setting jackdaw pid to %d\n", pd_shm->jackdaw_pid); */
    
    /* initiate_handshake(); */

    /* sem_wait(audio_params_sem); */
    /* fprintf(stdout, "Buffers semname: %s\n", pd_shm->audio_buffers_semname); */
    /* fprintf(stdout, "Audio params semname: %s\n", pd_shm->audio_params_semname); */
    
}

extern Project *proj;
void *pd_jackdaw_record_on_thread(void *arg)
{

    fprintf(stdout, "PD JACKDAW RECORD ON THREAD\n");
    AudioConn *conn = (AudioConn *)arg;
    /* SDL_Delay(10); */
    if (!connection_open) {
	fprintf(stderr, "Error: cannot record from pd; connection not open\n");
	return NULL;
    }

    if (pd_blocksize == 0) {
	if (sem_wait(audio_params_sem) == -1) {
	    perror("Error: audio params semwait failed");
	    return NULL;
	}
	pd_blocksize = pd_shm->pd_blocksize;
	fprintf(stdout, "PD BLOCKSIZE: %d\n", pd_blocksize);
    }

    PdConn *pdconn = &conn->c.pd;
    while (1) {
	/* fprintf(stdout, "about to check proj rec\n"); */
	if (!session->playback.recording) {
	    break;
	}
	while (sem_trywait(audio_buffers_write_sem) == 0) {};
	sem_post(audio_buffers_write_sem);

	/* fprintf(stdout, "done check proj rec\n"); */
	pd_shm->recording = true;
	
	/* sem_post(audio_params_sem); */
	/* sem_post(audio_buffers_write_sem); */

	/* fprintf(stdout, "pd conn? %p, at pos %d/%d\n", pdconn, pdconn->write_bufpos_sframes, pdconn->rec_buf_len_sframes); */
	if (pdconn->write_bufpos_sframes + pd_blocksize >= pdconn->rec_buf_len_sframes) {
	    Clip *clip = conn->current_clip;
	    if (clip) {
		copy_conn_buf_to_clip(clip, PURE_DATA);
		pdconn->write_bufpos_sframes = 0;
	    }
	}
	/* int i=0; */
	/* /\* while (sem_trywait(audio_buffers_read_sem) == 0) {if (i !=0 ) {fprintf(stdout, "decr %d\n", i);} i++;}; *\/ */
	/* /\* fprintf(stdout, "about to write to buffers %d\n", i); *\/ */
	/* i++; */
	/* i%= 10; */

	sem_wait(audio_buffers_read_sem);
	
	/* if (SDL_TryLockMutex(pdconn->buf_lock) == SDL_MUTEX_TIMEDOUT) { */
	/*     /\* SDL_UnlockMutex(pdconn->buf_lock); *\/ */
	/*     break; */
	/* } */
	memcpy(pdconn->rec_buffer_L + pdconn->write_bufpos_sframes, pd_shm->L, pd_blocksize * sizeof(float));
	/* if (SDL_TryLockMutex(pdconn->buf_lock) == SDL_MUTEX_TIMEDOUT) { */
	/*     /\* SDL_UnlockMutex(pdconn->buf_lock); *\/ */
	/*     break; */
	/* } */

	memcpy(pdconn->rec_buffer_R + pdconn->write_bufpos_sframes, pd_shm->R, pd_blocksize * sizeof(float));
	pdconn->write_bufpos_sframes += pd_blocksize;
	/* fprintf(stdout, "pdconn writebufpos: %d\n", pdconn->write_bufpos_sframes); */
	/* fprintf(stdout, "wrote to bufs!\n"); */
    }
    /* copy_pd_buf_to_clip(conn->current_clip); */
    pdconn->write_bufpos_sframes = 0;
    fprintf(stdout, "exit record\n");
    pd_shm->recording = false;
    /* sem_post(audio_params_sem); */
    fprintf(stdout, "EXIT PD JACKDAW RECORD ON THREAD\n");
    return NULL;

}
