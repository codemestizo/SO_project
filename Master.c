#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "Utility.h"



/* Semaforo per segnalare che i processi figli sono pronti */
#define ID_READY      0
// impostare num risorse.. merci
int sem_id;
pid_t idPorto;
pid_t idNave;
#define NUM_PROCESSI (SO_PORTI + SO_NAVI) //IL QUANTITATIVO DI PROCESSI FIGLI
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }

//serie di test error

void print_resource()
{
    /*unsigned short sem_vals[NUM_RISORSE], i;

    semctl(sem_id, 0 /* ignored *//*, GETALL, sem_vals);
    printf("READY  OLIO  UOVO  SALE  POMO PANCE  CIPO  PATA PASTA PENTO FUOCO  PADE\n");
    for (i=0; i<NUM_RISORSE; i++) {
        printf("%5d ", semctl(sem_id, i, GETVAL));
    }
    printf("\n");*&*/
}


int main(){


    struct sigaction sa;
    static int sem_id;
    int i,j,child_pid,status,kid_succ=0,kid_fail=0,type;
    struct sembuf b;
    struct timespec now;
    sigset_t my_mask;

    //fillAndCreate_resource(); // istanzia tutte le varie code,semafori,memorie condivise necessarie PER TUTTI i processi(keyword static)

// Read time at the beginning
    //time_start = time(NULL);

    // Create NUM_PROC processes
    for (i=0; i<SO_PORTI; i++) {
        switch (fork()) {
            case -1:
                /* Handle error */
                TEST_ERROR;
                exit(EXIT_FAILURE);
                //execve che chiama il file porto
            case 0:

                //padre

                exit(0);
                break;

            default:
                break;
        }
    }
    for (i=0; i<SO_NAVI; i++) {
        switch (fork()) {
            case -1:
                /* Handle error */
                TEST_ERROR;
                exit(EXIT_FAILURE);
                //execve che chiama il file nave
            case 0:

                //padre

                exit(0);
                break;

            default:
                break;
        }
    }



    /*
     * Attendi che i figli abbiano terminato
     * correttamente  oppure che non ce
     * l'abbiano fatta. Fra TIMEOUT secondi, pero`, si smette.
     */
    print_resource();//stampa i dati della simulazione, da analizzare

    //sezione dedicata alla terminazione della simulazione
    //sa.sa_handler = handle_signal;
    sa.sa_flags = 0; 	/* No special behaviour */
    sigemptyset(&my_mask);
    sa.sa_mask = my_mask;
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    alarm(SO_DAYS);


    //LA MIA IDEA ERA DI DIVIDERE PORTI E NAVI, FACENDO DUE CICLI FOR...

    /* Now let's wait for the termination of all kids */
    //utile per stampare lo stato finale della simulazione
    while ((child_pid = wait(&status)) != -1) {
        printf("PARENT: PID=%d. Got info of child with PID=%d, status=0x%04X\n", getpid(), child_pid,status);
    }

    /*time_end = time(NULL);
    fprintf(stderr,"Total time: %ld (sec)\n", time_end-time_start);*/
}