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

#include "utility.h"


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

    portArrayId = shmget(IPC_PRIVATE, 5*sizeof(int), IPC_CREAT);


    /*int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(IPC_PRIVATE,size,0666 | IPC_CREAT);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        perror(strerror(errno));
    }

*/
    printf("creata e inizializzata l'area di memoria condivisa con ID = %d\n", portArrayId);

/*
    portDefinition *portArrays =shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }*/


    //test per vedere se salva


    //array = (sizeof(int)*5);



   // createPortArray();

/*
    if (shmdt(portArrays) == -1) {
        fprintf(stderr, "%s: %d. Errore in shmdt #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }*/
    printf("Area di memoria condivisa sganciata.\n");


/*
    semPortArrayId=  semget(keySemPortArray,1,0600); //creo semafori della sh
    if(semPortArrayId == -1){
        printf("errore durante la creazione dei semafori sh");
        perror(strerror(errno));
    }


    semMessageQueueId=  semget(keySemMessageQueue,SO_PORTI,0600); //creo semafori della coda di messaggi
    if(semMessageQueueId == -1){
        printf("errore durante la creazione dei semafori message");
        perror(strerror(errno));
    }

    messageQueueId=msgget(keyMessageQueue, IPC_CREAT | IPC_EXCL| 0666)  ; //creo coda di messaggi

    if(messageQueueId == -1){
        printf("errore durante la creazione della coda messaggi");
        perror(strerror(errno));
    }


    //generaMerce();
    /*for(int i = 0;i<SO_PORTI;i++){
        printf("%d  x \n",portArrays[i].x);
        printf("%d  y \n",portArrays[i].y);
        printf("%d \n",portArrays[i].idPorto);

    }

*/
}

void clean(int queueId){ //dealloca dalla memoria

    if (msgctl(queueId, IPC_RMID, NULL)== -1) { //cancella coda di messaggi
        fprintf(stderr, "Non posso cancellare la coda messaggi.\n");
        exit(EXIT_FAILURE);
    }

}


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


int stampaStatoMemoria(int portArrayId) {
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
       // printf("x del primo porto: %d",portArrays[0].x);
        printf("Ultimo processo shmat/shmdt: %d\n",buf.shm_lpid);
        printf("Processi connessi: %ld\n",buf.shm_nattch);
        printf("\n");



        printf("\nRead to memory succesful--\n");

        return 0;
    }
}

void nigga(){
    int a=5,b=7;
    int buffer[1];
    int *point;

    shmid=shmget(IPC_PRIVATE , sizeof(buffer),0666);
    point=(int *)shmat(shmid,NULL,0);
    point[0]=a;
    point[1]=b;
    printf("FIRST = %d\nSECOND = %d",point[0],point[1]);
    fflush(stdout);
    sleep(1);
    shmdt(point);
}



int main(){
    struct sigaction sa;
    static int sem_id;
    int i,j,child_pid,status,kid_succ=0,kid_fail=0,type;
    struct sembuf b;
    struct timespec now;
    sigset_t my_mask;
    printf("pRIMA DI CREATIPCKEYS");
    //createIPCKeys();

    fillAndCreate_resource(); // istanzia tutte le varie code,semafori,memorie condivise necessarie PER TUTTI i processi(keyword static)
    //stampaStatoMemoria( portArrayId);
// Read time at the beginning
    //time_start = time(NULL);
    nigga();
    testo();

    printf("Id  della sm: %d \n",portArrayId);
    printf("Id del semaforo della sm: %d \n",semPortArrayId);
    printf("Id del semaforo delle code: %d \n",semMessageQueueId);
    printf("Id  delle code: %d \n",messageQueueId);
    //setPorto( portArrays);
   /* for(i = 0;i<SO_PORTI;i++){
        printf("%d \n",portArrays[i].x);
        printf("%d \n",portArrays[i].y);
        printf("%d \n",portArrays[i].idPorto);
        for(j=0;j<SO_MERCI;j++){
            printf("%d \n",portArrays[i].merce[j].offertaDomanda);
            printf("%d \n",portArrays[i].merce[j].vitaMerce);
            printf("%f \n",portArrays[i].merce[j].quantita);
            printf("%d \n",portArrays[i].merce[j].nomeMerce);
        }
    }*/

    // Create NUM_PROC processes
   /**/ for (i=0; i<SO_PORTI; i++) { //execve non vede il file, sistemato però (andava messo in case 0 e non -1) //TODO FIXARE execve
        switch (fork()) {
            case 0:
                /* Handle error */
                TEST_ERROR;
                char *argv[]={NULL};
                char* command = "./porto.c";
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
                printf("sono prima di exec Nave \n");
                char *argv[]={NULL};
                char* command = "./nave";
                if(execvp(command, argv)==-1){
                    printf("errore durante l'esecuzione del execve per la nave \n");
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



    /*
     * Attendi che i figli abbiano terminato
     * correttamente  oppure che non ce
     * l'abbiano fatta. Fra TIMEOUT secondi, pero`, si smette.
     */
    //print_resource();//stampa i dati della simulazione, da analizzare

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


//clean(messageQueueId);


}