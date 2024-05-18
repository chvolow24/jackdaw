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
/* #include "pure_data.h" */

#define MAX_SEMNAME 255
#define MAX_BUFLEN 4096

#define PD_JACKDAW_SHM_PIDS_NAME "/pd_jackdaw_pids"
#define PD_JACKDAW_SHM_NAME "/pd_jackdaw_shm"
#define PD_AUDIOPARAMS_SEMNAME "/pd_jackdaw_audioparam_sem"
#define PD_AUDIOBUFFERS_SEMNAME "/pd_jackdaw_audiobuf_sem"


struct pd_jackdaw_shm_pids {
    pid_t jackdaw_pid;
    pid_t pd_pid;
};

typedef struct pd_input {
    char secret[255];
    uint8_t secret_len;
    bool handshake_done;
    char audio_params_semname[MAX_SEMNAME];
    uint8_t audio_params_semname_len;
    char audio_buffers_semname[MAX_SEMNAME];
    uint8_t audio_buffers_semname_len;
    bool writer_open;
    uint16_t pd_blocksize;
    float L[MAX_BUFLEN];
    float R[MAX_BUFLEN];
} PdInput;


struct pd_jackdaw_shm_pids *pids;
PdInput *pd_shm;
bool handshake_done = false;
bool connection_open = false;
uint16_t pd_blocksize = 0;

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
    
    /* Indicate that shm can be safely opened */
    kill(pids->pd_pid, SIGUSR1);
    fprintf(stdout, "Sent signal 1 to pd; pd can now open main shm\n");

    /* Unlink PIDS shared memory object; no more signals will be passed. */
    shm_unlink(PD_JACKDAW_SHM_PIDS_NAME);
    connection_open = true;
}


static void handle_signal1_from_pd(int signo)
{
    if (connection_open) {

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
     }
}

int pd_jackdaw_shm_init()
{
    signal(SIGUSR1, handle_signal1_from_pd);
    signal(SIGUSR2, handle_signal2_from_pd);

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

    /* if (sem_unlink(PD_AUDIOPARAMS_SEMNAME) != 0) { */
    /* 	perror("Unable to unlink named semaphore"); */
    /* } */
    /* if (sem_unlink(PD_AUDIOBUFFERS_SEMNAME) != 0) { */
    /* 	perror("Unable to unlink named semaphore"); */
    /* } */

    /* sem_t *audio_params_sem = sem_open(pd_shm->audio_params_semname, O_CREAT | O_EXCL, 0666, 0); */
    /* if (audio_params_sem == SEM_FAILED) { */
    /* 	perror("Failed to open params semaphore\n"); */
    /* } */

    /* sem_t *audio_buffers_sem = sem_open(pd_shm->audio_buffers_semname, O_CREAT | O_EXCL, 0666, 0); */
    /* if (audio_buffers_sem == SEM_FAILED) { */
    /* 	perror("Failed to open buffer semapahore\n"); */
    /* } */

    /* sem_wait(audio_params_sem); */
    /* fprintf(stdout, "Buffers semname: %s\n", pd_shm->audio_buffers_semname); */
    /* fprintf(stdout, "Audio params semname: %s\n", pd_shm->audio_params_semname); */
    
}


