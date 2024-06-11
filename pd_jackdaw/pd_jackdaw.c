#include <stdio.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "m_pd.h"
#include "string.h"

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

pid_t jackdaw_pid = 0;

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
bool handshake_complete = false;
bool connection_open = false;
bool blocksize_established = false;
sem_t *audio_params_sem = NULL;
sem_t *audio_buffers_write_sem = NULL;
sem_t *audio_buffers_read_sem = NULL;

static t_class *pd_jackdaw_tilde_class = NULL;

typedef struct _pd_jackdaw_tilde {
    t_object x_obj;
    t_inlet *iR;
    t_outlet *out;
    t_float dummy_f;
} t_pd_jackdaw_tilde;


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


void pd_jackdaw_tilde_bang(t_pd_jackdaw_tilde *x)
{
}

void handle_signal2_from_jackdaw(int signo)
{
    post("Received signal 2 from jackdaw...");
    /* Pure Data receiving SIGUSR2 from jackdaw signifies the end of the handshake */
    post("HANDSHAKE DONE.");
    jackdaw_pid = pids->jackdaw_pid;
    handshake_complete = true;
}

void handle_signal1_from_jackdaw(int signo)
{
    post("received signal 1 from jackdaw...");
    if (connection_open) {
	post("Jackdaw appears to have closed. Terminating connection.");
	connection_open = false;
	handshake_complete = false;
	return;
    }
    if (handshake_complete) {
	/* This condition indicates that jackdaw has created the main shm object and is ready to share data */
	int fd = shm_open(PD_JACKDAW_SHM_NAME, O_RDWR);
	if (fd == -1) {
	    post("Error opening main shm: %s", strerror(errno));
	}
	pd_shm = (PdInput *)mmap(NULL, sizeof(PdInput), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (pd_shm == MAP_FAILED) {
	    post("Error mapping main shm: %s", strerror(errno));
	}
	post("successfully opened main shm");
	char secret[255];
	strncpy(secret, pd_shm->secret, pd_shm->secret_len);
	secret[pd_shm->secret_len] = '\0';
	post("The secret is: %s", pd_shm->secret);
	audio_params_sem = sem_open(PD_AUDIOPARAMS_SEMNAME, O_EXCL);
	if (audio_params_sem == SEM_FAILED) {
	    post("Failed to open params semaphore: %s", strerror(errno));
	}
	audio_buffers_write_sem = sem_open(PD_AUDIOBUFFERS_WRITE_SEMNAME, O_EXCL);
	if (audio_buffers_write_sem == SEM_FAILED) {
	    post("Failed to open buffer semapahore: %s", strerror(errno));
	}
	audio_buffers_read_sem = sem_open(PD_AUDIOBUFFERS_READ_SEMNAME, O_EXCL);
	if (audio_buffers_read_sem == SEM_FAILED) {
	    post("Failed to open buffer semapahore: %s", strerror(errno));
	}
	jackdaw_pid = pids->jackdaw_pid;
	connection_open = true;
    } else {
	jackdaw_pid = pids->jackdaw_pid;
	post("Received sig1 from jackdaw; pd started first. responding...\n");
	kill(jackdaw_pid, SIGUSR2);
    }

}

static void initiate_handshake()
{
    if (jackdaw_pid != 0 && validate_pid(jackdaw_pid, "jackdaw")) {
	post("Validated jackdaw pid. Sending signal");
	kill(jackdaw_pid, SIGUSR1);
    } else {
	post("Jackdaw PID is zero or could not be validated. (PID: %d)", jackdaw_pid);
    }

}

void *pd_jackdaw_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    /* FILE *errlog = fopen("/Users/charlievolow/desktop/err.log", "w"); */
    /* fprintf(errlog, "Initiating tilde new...\n"); */

    if (!connection_open) {
	signal(SIGUSR1, handle_signal1_from_jackdaw);
	signal(SIGUSR2, handle_signal2_from_jackdaw);
	int fd = shm_open(PD_JACKDAW_SHM_PIDS_NAME, O_CREAT | O_RDWR, 0666);
	if (fd == -1) {
	    /* fprintf(errlog, "Error in shm open: %s", strerror(errno)); */
	    post("Error in shm open: %s", strerror(errno));
	    return NULL;
	}
	// Set the size of the shared memory object
	if (ftruncate(fd, sizeof(PdInput)) == -1) {
	    /* fprintf(errlog, "Error in ftruncate: %s", strerror(errno)); */
	    post("Error in ftruncate: %s", strerror(errno));
	    /* return NULL; */
	}
	post( "in init, jackdaw pid is %d", jackdaw_pid);
	if (jackdaw_pid == 0) {
	    pids = (struct pd_jackdaw_shm_pids *)mmap(NULL, sizeof(struct pd_jackdaw_shm_pids), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	    if (pids == MAP_FAILED) {
		/* fprintf(errlog, "Error mapping PID struct: %s", strerror(errno)); */
		post("Error mapping PID struct: %s", strerror(errno));
	    } else {
		pids->pd_pid = getpid();
		jackdaw_pid = pids->jackdaw_pid;
	    }
	    close(fd);
	    post("Done with init: created pids shm, set pid, and assigned sig handlers");
	}
	initiate_handshake();
    }


    /* sem_t *audio_params_sem = sem_open(PD_AUDIOPARAMS_SEMNAME, O_EXCL); */
    /* if (audio_params_sem == SEM_FAILED) { */
    /* 	post("Unable to open audio params sem:"); */
    /* } */
    /* sem_t *audio_buffers_sem = sem_open(PD_AUDIOBUFFERS_SEMNAME, O_EXCL); */
    /* if (audio_buffers_sem == SEM_FAILED) { */
    /* 	post("Unable to open audio buffer sem:"); */
    /* } */


    
    /* strcpy(pd_shm->audio_params_semname, PD_AUDIOPARAMS_SEMNAME); */
    /* strcpy(pd_shm->audio_buffers_semname, PD_AUDIOBUFFERS_SEMNAME); */

    /* pd_shm->pd_blocksize = 512; */
    /* sem_post(audio_params_sem); */

    t_pd_jackdaw_tilde *x = (t_pd_jackdaw_tilde *)pd_new(pd_jackdaw_tilde_class);
    for (int i=0; i<argc; i++) {
	post("pointer: %p\n", argv+i);
	post("atom getint: %d\n", atom_getint(argv+i));
    }
    /* inlet_new(t_object *owner, t_pd *dest, t_symbol *s1, t_symbol *s2) -> struct _inlet */
    x->iR = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    x->out = outlet_new(&x->x_obj, &s_signal);
    post("done new method");
    return (void *)x;
}

void pd_jackdaw_tilde_free(t_pd_jackdaw_tilde *x)
{
    inlet_free(x->iR);
    outlet_free(x->out);
    if (jackdaw_pid != 0) {
	kill(jackdaw_pid, SIGUSR1);
    }
    handshake_complete = false;
    connection_open = false;
    post("Connection to jackdaw closed");
}

t_int *pd_jackdaw_tilde_perform(t_int *w)
{
    t_pd_jackdaw_tilde *x = (t_pd_jackdaw_tilde *)(w[1]);
    t_sample    *in1 =      (t_sample *)(w[2]);
    t_sample    *in2 =      (t_sample *)(w[3]);
    t_sample    *out =      (t_sample *)(w[4]);
    int            n =             (int)(w[5]);
    if (connection_open) {
	if (!blocksize_established) {
	    post("posting pd blocksize: %d", n);
	    pd_shm->pd_blocksize = n;
	    if (sem_post(audio_params_sem) == -1) {
		post("Unable to post to audio params sem: %s", strerror(errno));
	    }
	    blocksize_established = true;
	}

	if (pd_shm->recording) {
	    /* post("connection open, recording"); */
	    int i=0;
	    int semval = -1;
	    /* post("Wait buf write..."); */
	    sem_wait(audio_buffers_write_sem);
	    /* post("done wait"); */
	    memcpy(pd_shm->L, in1, sizeof(t_float) * n);
	    memcpy(pd_shm->R, in2, sizeof(t_float) * n);
	    while (sem_trywait(audio_buffers_read_sem) == 0) {}
	    if (sem_post(audio_buffers_read_sem) == -1) {
		post("Post to buffer sem failed: %s", strerror(errno));
	    }
	} else {
	    /* post("NOT RECORDING"); */
	}
    }
    return w + 6;
}

void pd_jackdaw_dsp(t_pd_jackdaw_tilde *x, t_signal **sp)
{
    post("call to dsp add");
    dsp_add(pd_jackdaw_tilde_perform, 5, x,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
    post("call to dsp add complete");
}

void pd_jackdaw_setup(void)
{
    pd_jackdaw_tilde_class = class_new(gensym("pd_jackdaw~"),
				 (t_newmethod)pd_jackdaw_tilde_new,
				 (t_method)pd_jackdaw_tilde_free,
				 sizeof(t_pd_jackdaw_tilde),
				 CLASS_DEFAULT,
				 A_GIMME,
				 0);
    CLASS_MAINSIGNALIN(pd_jackdaw_tilde_class, t_pd_jackdaw_tilde, dummy_f);
    /* class_addbang(pd_jackdaw_tilde_class, pd_jackdaw_tilde_bang); */

    /* class_addmethod(pd_jackdaw_tilde_class, */
    /*   (t_method)pd_jackdaw_tilde_reset, */
    /*   gensym("reset"), 0); */

    /* class_addmethod(pd_jackdaw_tilde_class, */
    /* 		    (t_method)pd_jackdaw_tilde_ten, */
    /* 		    gensym("ten"), */
    /* 		    A_DEFFLOAT, */
    /* 		    0); */
    class_addmethod(pd_jackdaw_tilde_class,
		    (t_method)pd_jackdaw_dsp,
		    gensym("dsp"),
		    A_CANT,
		    0);

}
