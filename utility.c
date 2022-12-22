//
// Created by daddy on 12/13/22.
//
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
    if(keySemMessageQueue == -1){
        TEST_ERROR
        perror("errore keySemMessageQueue");
    }
}

void createPortArray(){

    int i,j;

    //portArray->ports = (portDefinition *)malloc(SO_PORTI * sizeof(portDefinition));
    for(i=0;i<SO_PORTI;i++){
        portArrays[i].x = 4;
        portArrays[i].y = 0;
        portArrays[i].idPorto = 0;
        portArrays[i].semIdBanchinePorto = 0;
        for(j=0;j<SO_MERCI;j++){
            portArrays[i].merce[j].offertaDomanda = 2;//0 = domanda, 1 = offerta, 2 = da assegnare
            portArrays[i].merce[j].vitaMerce = 0;
            portArrays[i].merce[j].nomeMerce = 0;
            portArrays[i].merce[j].quantita = 0;
        }
        j=0;
    }
}

//CONTROLLA SE LA NAVE E' SUL PORTO
int controlloPosizione( int x, int y, portDefinition *portArrays){ //in teoria è giusto TODO check se è giusto (a livello di come punta e logica)
    int portoAttuale; //contatore
    for(portoAttuale=0;portoAttuale<SO_PORTI;portoAttuale++){

        if( portArrays[portoAttuale].x == x && portArrays[portoAttuale].y == y){
            return portArrays[portoAttuale].idPorto; //ritorna il porto n su cui si trova la nave o dovrò tornare il PID di quel porto?
        }
    }
    return -1;// se -1 non è su nessun porto
}


void generaMerce(){ //metodo per generare le merci randomicamente con quantità +-simili
    int sum=0;
    int leng=SO_PORTI*SO_MERCI;
    int i=0;
    srand(time(NULL));
    int array[leng];
    array[i]=(rand() %  SO_FILL/leng);
    sum+=array[i];
    for ( i=1; i<leng; i++){
        if (!(i == leng-1)) {
            array[i]=( (SO_FILL-sum)/(leng-i)+i) ;
            sum+=array[i];}
        else
            array[i]= (SO_FILL-sum);
    }
}


//codice preso dalle slide sull'utilizzo dei semafori,NON di nostra inventiva

// Initialize semaphore to 1 (i.e., "available")
int initSemAvailable(int semId, int semNum) {
    union semun arg;
    arg.val = 1;
    return semctl(semId, semNum, SETVAL, arg);
}

// Reserve semaphore - decrement it by 1
int reserveSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}
// Release semaphore - increment it by 1
int releaseSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}

/*int main(int argc, char** argv){ //ricordatevi che questo main è temporaneo, una volta sicuri che funziona il file utility va eliminato (il main)

    createIPCKeys();

    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;

    portArrayId = shmget(IPC_PRIVATE,size,0666);// crea la shared memory con shmget
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        perror(strerror(errno));
    }
    portArrays = shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }

    createPortArray(portArrays);

    int test = controlloPosizione(0,0,portArrays);

    for(int i = 0;i<SO_PORTI;i++){
        printf("%d \n",portArrays[i].x);
        printf("%d \n",portArrays[i].y);
        printf("%d \n",portArrays[i].idPorto);
        for(int j=0;j<SO_MERCI;j++){
            printf("%d \n",portArrays[i].merce[j].offertaDomanda);
            printf("%d \n",portArrays[i].merce[j].vitaMerce);
            printf("%f \n",portArrays[i].merce[j].quantita);
            printf("%d \n",portArrays[i].merce[j].nomeMerce);
        }
    }
}*/


