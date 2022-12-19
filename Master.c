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
#define NUM_RISORSE
/* ID dell'IPC del semaforo e` globale */
int sem_id;
pid_t idPorto;
pid_t idNave;
#define NUM_PROCESSI= SO_PORTI+SO_NAVI; //IL QUANTITATIVO DI PROCESSI FIGLI
#define ERRORI  if(errno){
fprintf(stderr,\ "%s:%d:PID=%5d:Error %d (%s)\n",\ __FILE__,\__LINE__,\getpid(),\errno,\strerror(errno));
//serie di test error
int main(){


    struct sigaction sa;
    int i,status,kid_succ=0,kid_fail=0,type;
    struct sembuf b;
    struct timespec now;
    sigset_t my_mask;
/*semaforo per iniz. le ris; ris messe sss proc pronti*/
    sem_id
            sem_id = semget(IPC_PRIVATE, NUM_RISORSE, 0600);
    TEST_ERROR;
    reset_sem(sem_id);
    b.sem_flag=0; //esclusione flag sul semaforo

// Read time at the beginning
    //time_start = time(NULL);

    // Create NUM_PROC processes
    for (j=0; j<SO_PORTI; i++) {
        for (i=0; i<SO_NAVI; i++) {
            switch (fork()) {
                case -1:
                    /* Handle error */
                    TEST_ERROR;
                    exit(EXIT_FAILURE);
                    //FIGLIO pronto
                case 0:
                    b.sem_num = ID_READY;
                    b.sem_op = -1;
                    semop(sem_id, &b, 1);
                    //printf("READY:%d\n",semctl(sem_id, ID_READY, id_Nave));

                    b.sem_num = ID_READY;
                    b.sem_op = 0;
                    semop(sem_id, &b, 1);

                    exit(0);
                    break;

                default:
                    break;
            }
        }
        switch (fork()) {
            case -1:
                /* Handle error */
                TEST_ERROR;
                exit(EXIT_FAILURE);
                //FIGLIO pronto
            case 0:
                b.sem_num = ID_READY;
                b.sem_op = -1;
                semop(sem_id, &b, 1);
                //printf("READY:%d\n",semctl(sem_id, ID_READY, id_Porto));

                b.sem_num = ID_READY;
                b.sem_op = 0;
                semop(sem_id, &b, 1);

                exit(0);
                break;

                //qui switch del type con uso del make
                //ad es: switch (tipo) {
                //case 0:
                //make_pata_fritte();
            default:
                break;
        }
    }


    /*
     * Attendi che i figli abbiano terminato
     * correttamente  oppure che non ce
     * l'abbiano fatta. Fra TIMEOUT secondi, pero`, si smette.
     */
    fill_resource(sem_id);
    print_resource();
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0; 	/* No special behaviour */
    sigemptyset(&my_mask);
    sa.sa_mask = my_mask;
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    alarm(TIMEOUT);

    /*
     * Il padre aspetta che tutti  siano pronti prima di dargli le
     * risorse della merce":  soltanto dopo che tutti  i NUM_PROC
     * figli avranno eseguito la loro semop che incrementa di 1 la
     * risorsa, si potranno consumare NUM_PROC risorse
     */
    b.sem_num = ID_READY;
    b.sem_op = -1;
    semop(sem_id, &b, 1);
    b.sem_num = ID_READY;
    b.sem_op = 0;
    semop(sem_id, &b, 1);


    //LA MIA IDEA ERA DI DIVIDERE PORTI E NAVI, FACENDO DUE CICLI FOR...

    /* Now let's wait for the termination of all kids */
    while ((child_pid = wait(&status)) != -1) {
        printf("PARENT: PID=%d. Got info of child with PID=%d, status=0x%04X\n", getpid(), child_pid,status);
    }

    /*time_end = time(NULL);
    fprintf(stderr,"Total time: %ld (sec)\n", time_end-time_start);*/
}
