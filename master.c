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
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>




#include "utility.h"

struct stat st;
/* Semaforo per segnalare che i processi figli sono pronti */
#define ID_READY      0
// impostare num risorse.. merci
int sem_id;
pid_t idPorto;
pid_t idNave;
#define NUM_PROCESSI (SO_PORTI + SO_NAVI) //IL QUANTITATIVO DI PROCESSI FIGLI
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }

//serie di test error



void fillAndCreate_resource(){

    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        perror(strerror(errno));
    }

    portArrays =shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }

//creo la fifo perricevere le info di cosa fanno i processi


    if (stat("fifo_name1", &st) != 0)
        mkfifo("fifo_name1", 0666);
    else{
        unlink("fifo_name1");
        mkfifo("fifo_name1", 0666);
    }



    semPortArrayId=  semget(keySemPortArray,1,IPC_CREAT | 0666); //creo semafori della sh
    if(semPortArrayId == -1){
        printf("errore durante la creazione dei semafori sh");
        perror(strerror(errno));
    }



    semMessageQueueId=  semget(keySemMessageQueue,SO_PORTI,IPC_CREAT | 0666); //creo semafori della coda di messaggi
    if(semMessageQueueId == -1){
        printf("errore durante la creazione dei semafori message");
        perror(strerror(errno));
    }

    messageQueueId=msgget(keyMessageQueue, IPC_CREAT | 0666)  ; //creo coda di messaggi

    if(messageQueueId == -1){
        printf("errore durante la creazione della coda messaggi");
        perror(strerror(errno));
    }


}

void clean(){ //dealloca dalla memoria
    void *mem = shmat(semPortArrayId, 0, 0);
    shmdt(mem);
/* 'remove' shared memory segment */
    shmctl(semPortArrayId, IPC_RMID, NULL);
    if (msgctl(messageQueueId, IPC_RMID, NULL)== -1) { //cancella coda di messaggi
        fprintf(stderr, "Non posso cancellare la coda messaggi.\n");
        exit(EXIT_FAILURE);
    }

}



int stampaStatoMemoria() {
    printf("prova");
    struct shmid_ds buf;

    if (shmctl(portArrayId,IPC_STAT,&buf)==-1) {
        fprintf(stderr, "%s: %d. Errore in shmctl #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    } else {
        printf("\nSTATISTICHE\n");
        printf("AreaId: %d\n",portArrayId);
        printf("Dimensione: %ld\n",buf.shm_segsz);
        printf("Ultima shmat: %s\n",ctime(&buf.shm_atime));
        printf("Ultima shmdt: %s\n",ctime(&buf.shm_dtime));
        printf("Ultimo processo shmat/shmdt: %d\n",buf.shm_lpid);
        printf("Processi connessi: %ld\n",buf.shm_nattch);
        printf("\n");
        printf("\nRead to memory succesful--\n");

        return 0;
    }
}

void reportGiornaliero(){
    int  s2c, c=0;
    char buf[1024];
    int sep=0;
    char delim[] = "|";
    char fifo_name1[] = "/tmp/fifo";
    int sellbuy=0;
    int saltaporto=1;
    s2c= open(fifo_name1, O_RDONLY | O_NONBLOCK);

    // receive messages
    while (c<SO_MERCI * SO_PORTI) {

        if (read(s2c, &buf, sizeof(char) * 25) > 0) {
           // char str[strlen(buf)];
           //
           // printf(" il buf vale: %s", buf);
            //strncpy(str, buf, strlen(buf));

            char *ptr = strtok(buf, delim);
            sep=0;
            sep++;
            if(saltaporto==SO_MERCI){
                saltaporto=0;
                sellbuy=1;
            }
            while (ptr != NULL) {
                if (sep == 1) {
                    printf("PORTO NUMERO: '%s' ", ptr);
                   // printf("zioperaaa1");

                } else if (sep == 2) {
                    printf("Merce numero: '%s' : ", ptr);
                   // printf("zioperaaa2");
                } else if (sep == 3) {
                    printf("In quantita' pari a  '%s' tonnellate ", ptr);
                } else if (sep == 4) {
                    printf(" e' richieta/offerta/non ( '%s' ) ", ptr);
                }else if (sep == 5) {
                    printf(" con '%s' giorni di vita rimanente ", ptr);
                }else if (sep == 6 && sellbuy==1) {
                    printf(" \n Oggi sono state ricevute %s  tonnellate di merce \n", ptr);
                }else if (sep == 7 && sellbuy==1) {
                    printf(" \n Oggi sono state vendute %s  tonnellate di merce \n ", ptr);
                }
                ptr = strtok(NULL, delim);
                sep++;
            }
            printf("\n");
            //sleep(0.1);
            c++;
            sellbuy=0;
            saltaporto++;
            if (c > SO_MERCI * SO_PORTI)
                break;
    }

    }
    printf("client exit successfully");

}





int main(){
    struct sigaction sa;
    static int sem_id;
    int i,j,child_pid,status,kid_succ=0,kid_fail=0,type;
    struct sembuf b;
    struct timespec now;
    sigset_t my_mask;
    printf("pRIMA DI CREATIPCKEYS");

    fillAndCreate_resource(); // istanzia tutte le varie code,semafori,memorie condivise necessarie PER TUTTI i processi(keyword static)
    



    //stampaStatoMemoria();


    printf("Id  della sm: %d \n",portArrayId);
    printf("Id del semaforo della sm: %d \n",semPortArrayId);
    printf("Id del semaforo delle code: %d \n",semMessageQueueId);
    printf("Id  delle code: %d \n",messageQueueId);


    // Create NUM_PROC processes
    /**/ for (i=0; i<SO_PORTI; i++) { //execve non vede il file, sistemato però (andava messo in case 0 e non -1) //TODO FIXARE execve
        sleep(0.5);
        switch (fork()) {
            case 0:

                /* Handle error */
                TEST_ERROR;
                char *argv[]={NULL};
                char* command = "./porto";
                if(execvp(command, argv)==-1){
                    printf("errore durante l'esecuzione del execve per il porto \n");
                    perror(strerror(errno));
                }
                exit(EXIT_FAILURE);


            case -1:
                //padre

                exit(0);
                break;
            default:
                break;
        }


    }

    for (i=0; i<SO_NAVI; i++) {
        switch (fork()) {
            case 0:
                /* Handle error */
                TEST_ERROR;
               // printf("sono prima di exec Nave \n");
                char *argv[]={NULL};
                char* command = "./nave";
                if(execvp(command, argv)==-1){
                    printf("errore durante l'esecuzione del execve per la nave \n");
                    perror(strerror(errno));
                }
                exit(EXIT_FAILURE);

            case -1:
                //reportGiornaliero();
                //padre

                exit(0);
                break;

            default:
                break;
        }

    }
    reportGiornaliero();


    /*
     * Attendi che i figli abbiano terminato
     * correttamente  oppure che non ce
     * l'abbiano fatta. Fra TIMEOUT secondi, pero`, si smette.
     */
    //print_resource();//stampa i dati della simulazione, da analizzare

    //sezione dedicata alla terminazione della simulazione
    //sa.sa_handler = handle_signal;
    //sa.sa_flags = 0; 	/* No special behaviour */
    /*sigemptyset(&my_mask);
    sa.sa_mask = my_mask;
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    alarm(SO_DAYS);*/


    /* Now let's wait for the termination of all kids */
    //utile per stampare lo stato finale della simulazione
    while ((child_pid = wait(&status)) != -1) {
        printf("PARENT: PID=%d. Got info of child with PID=%d, status=0x%04X\n", getpid(), child_pid,status);
    }

    /*time_end = time(NULL);
    fprintf(stderr,"Total time: %ld (sec)\n", time_end-time_start);*/


//clean(messageQueueId);
    clean();

}
