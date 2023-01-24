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

void startMeteo(int argc, char *argv[]) {

    createIPCKeys();

    semDaysId=  semget(keyGiorni,SO_PORTI+SO_NAVI,IPC_CREAT | 0666);
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    portArrays = shmat(portArrayId,NULL,0);

    int giorniSimulazione=0;
    int numeroNavi=0;
    int numeroPorti=0;
    int mortiGiornaliere=0;
    int naveRallentata=0;
    int portoRallentato=0;
    int naveAffondata=0;
    int pidPortoAlto=0;//indica il pid dell'ultimo porto
    int naviRallentate=0;
    int portiRallentati=0;
    int naviAffondate=0;
    srand(getpid());
    for(int i=0;i<SO_PORTI;i++){
        if(portArrays[i].idPorto>pidPortoAlto)
            pidPortoAlto=portArrays[i].idPorto;
    }
    numeroNavi=getpid()-1-pidPortoAlto;
    numeroPorti=getppid() - pidPortoAlto;
    mortiGiornaliere = 24/SO_MAELSTROM;
    while(giorniSimulazione<SO_DAYS){
        printf("Giorno per meteo: %d.\n",giorniSimulazione);
        naveRallentata = (rand() %  numeroNavi);
        if(kill(pidPortoAlto + naveRallentata + 1,SIGUSR1)==-1){
            TEST_ERROR;
            perror("errore durante l'invio del segnale da meteo per rallentare la nave");
        }
        naviRallentate++;
        portoRallentato = (rand() %  numeroPorti);
        if(kill(getppid() + portoRallentato + 1,SIGUSR1)==-1){
            TEST_ERROR;
            perror("errore durante l'invio del segnale da meteo per rallentare il porto");
        }
        portiRallentati++;
        for(int i=0;i<mortiGiornaliere;i++){
            naveAffondata = (rand() %  numeroNavi);
            if(kill(getppid() + naveAffondata + 1,SIGTERM)==-1){
                TEST_ERROR;
                perror("errore durante l'invio del segnale da meteo per terminare la nave");
            }
            naviAffondate++;
        }
        while(semctl(semDaysId,SO_PORTI-1,GETVAL) < giorniSimulazione+1){

        }
        giorniSimulazione++;
        if(giorniSimulazione==SO_DAYS-1)
            break;
    }

}