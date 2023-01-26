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
#include <sys/stat.h>
#include <fcntl.h>           /* Definition of AT_* constants */

#include "utility.h"

struct stat st;
int giorniSimulazione = 0;
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }


void createIPCKeys(){
    keyPortArray = ftok("master.c", 'u');
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyPortArray");
    }

    keySemPortArray = ftok("master.c", 'm');
    if(keySemPortArray == -1){
        TEST_ERROR
        perror("errore keySemPortArray");
    }
    keyMessageQueue = ftok("master.c", 'p');
    if(keyMessageQueue == -1){
        TEST_ERROR
        perror("errore keyMessageQueue");
    }
    keySemMessageQueue = ftok("master.c", 'n');
    if(messageQueueId == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }

    keyGiorni = ftok("master.c", 'o');
    if(semDaysId == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }

    keyStart = ftok("master.c", 'g');
    if(semPartiId == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }
}

void fillAndCreate_resource(){

    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        perror(strerror(errno));
    }

    portArrays =shmat(portArrayId,NULL,0); /*specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo*/
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }

/*creo la fifo perricevere le info di cosa fanno i processi*/


    if (stat("fifo_name1", &st) != 0)
        mkfifo("fifo_name1", 0666);
    else{
        unlink("fifo_name1");
        mkfifo("fifo_name1", 0666);
    }

    semPortArrayId=  semget(keySemPortArray,SO_PORTI,IPC_CREAT | 0666); /*creo semafori della sh*/
    if(semPortArrayId == -1){
        printf("errore durante la creazione dei semafori sh");
        perror(strerror(errno));
    }
    for(int i=0;i<SO_PORTI;i++){
        initSemAvailable(semPortArrayId,i);
    }

    semMessageQueueId=  semget(keySemMessageQueue,SO_PORTI,IPC_CREAT | 0666); /*creo semafori della coda di messaggi*/
    if(semMessageQueueId == -1){
        printf("errore durante la creazione dei semafori message");
        perror(strerror(errno));
    }


    semDaysId=  semget(keyGiorni,(SO_PORTI+SO_NAVI),IPC_CREAT | 0666); /*creo semafori gestione giorni*/
    if(semDaysId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }

    semPartiId=  semget(keyStart,1,IPC_CREAT | 0666); /*creo semaforo per far partire i giorni*/
    if(semPartiId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }



    messageQueueId=msgget(keyMessageQueue, IPC_CREAT | 0666); /*creo coda di messaggi*/

    if(messageQueueId == -1){
        printf("errore durante la creazione della coda messaggi");
        perror(strerror(errno));
    }
}


void clean(){ /*dealloca dalla memoria*/
    void *mem = shmat(portArrayId, 0, 0);

    if(semctl(semPortArrayId,SO_PORTI,IPC_RMID)==-1)
        TEST_ERROR;
    for(int r=0;r<SO_PORTI;r++){
        semctl(portArrays[r].semIdBanchinePorto,SO_BANCHINE,IPC_RMID);
    }
    shmdt(mem);

/* 'remove' shared memory segment */
    shmctl(portArrayId, IPC_RMID, NULL);
    if (msgctl(messageQueueId, IPC_RMID, NULL)== -1) { /*cancella coda di messaggi*/
        fprintf(stderr, "Non posso cancellare la coda messaggi.\n");
        exit(EXIT_FAILURE);
    }

    shmctl(portArrayId, IPC_RMID,0);

    /*azzero semafori dei giorni*/


    printf("\n ora pulisco i semafori dei processi");
    for(int j=0;j<=SO_NAVI+SO_PORTI-1;j++){
        while(semctl(semDaysId,j,GETVAL)!=0)
            reserveSem(semDaysId, j);
    }

    /* while (semctl(semPartiId, 0, GETVAL) != 0)
         reserveSem(semPartiId, 0);
 */
    printf("\nho pulito");
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
    s2c= open(fifo_name1, O_RDONLY );

    /* receive messages*/
    while (c<SO_MERCI * SO_PORTI) {

        if (read(s2c, &buf, sizeof(char) * 25) > 0) {
            printf("\nRicevo il buf: '%s' ", buf);
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
                } else if (sep == 2) {
                    printf("Merce numero: '%s' : ", ptr);
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
            c++;
            sellbuy=0;
            saltaporto++;
            if (c > SO_MERCI * SO_PORTI)
                break;
        }

    }
    printf("client exit successfully");
    close(s2c);
}


int main() {

    struct sigaction sa;
    union semun arrayValoriGiorni;
    static int sem_id;
    int i, j, child_pid, status, kid_succ = 0, kid_fail = 0, type;
    struct sembuf b;
    struct timespec now;
    sigset_t my_mask;


    createIPCKeys();
    fillAndCreate_resource(); /* istanzia tutte le varie code,semafori,memorie condivise necessarie PER TUTTI i processi(keyword static)*/

    clean();
    createIPCKeys();
    fillAndCreate_resource();

    /*creazione processi porto*/
    for (i = 0; i <SO_PORTI; i++) {
        sleep(0.5);
        switch (fork()) {
            case 0:
                /* Handle error */
                TEST_ERROR;
                char *argv[] = {NULL};
                char *command = "./porto";
                if (execvp(command, argv) == -1) {
                    printf("errore durante l'esecuzione del execve per il porto \n");
                    perror(strerror(errno));
                }
                exit(EXIT_FAILURE);


            case -1:
                /*padre*/
                break;
            default:
                break;
        }


    }
    sleep(SO_PORTI * 0.5);
    for (i = 0; i < SO_NAVI; i++) {
        sleep(0.01);
        switch (fork()) {
            case 0:
                /* Handle error */
                TEST_ERROR;
                char *argv[] = {NULL};
                char *command = "./nave";
                if (execvp(command, argv) == -1) {
                    printf("errore durante l'esecuzione del execve per la nave \n");
                    perror(strerror(errno));
                }
                exit(EXIT_FAILURE);

            case -1:

                /*padre*/
                exit(EXIT_FAILURE);
                break;

            default:
                break;
        }

    }

    switch (fork()) {

        case 0:
            /* Handle error */
            TEST_ERROR;
            char *argv[] = {NULL};
            char *command = "./meteo";
            if (execvp(command, argv) == -1) {
                printf("errore durante l'esecuzione del execve per il processo meteo \n");
                perror(strerror(errno));
            }
            exit(EXIT_FAILURE);

        case -1:
            /*padre*/
            exit(EXIT_FAILURE);
            break;

        default:
            break;
    }

    /* appena un figlio termina eseguo altre operazioni */
    while ((child_pid = wait(&status)) != -1) {
        printf("PARENT: PID=%d. Got info of child with PID=%d, status=0x%04X\n", getpid(), child_pid, status);
        if(child_pid>(getpid()+SO_PORTI)){
            struct sembuf sops;
            sops.sem_num = child_pid - SO_PORTI - getpid();
            sops.sem_op = SO_DAYS-1;
            sops.sem_flg = 0;
            if(semop(semDaysId, &sops, 1)==-1){
                TEST_ERROR
            }
            printf("incrementato semaforo giorni della nave %d a SO_DAYS-1",child_pid);
            printf("valore semaforo semDays per la nave %d: %d",child_pid,semctl(semDaysId, sops.sem_num, GETVAL));
        }
        else{
            struct sembuf sops;
            sops.sem_num = child_pid - getpid();
            sops.sem_op = SO_DAYS-1;
            sops.sem_flg = 0;
            if(semop(semDaysId, &sops, 1)==-1){
                TEST_ERROR
            }
            printf("incrementato semaforo giorni del porto %d a SO_DAYS-1",child_pid);
        }
    }
    printf("\nDAJE ROOMAAAA");
    clean();

}
