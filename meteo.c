#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>

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
    keyReport = ftok("master.c", 'r');
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyPortArray");
    }
}

void startMeteo(int argc, char *argv[]) {
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    int killShip=0, giorniSimulazione=0, naveRallentata=0, portoRallentato=0, naveAffondata=0, naviRallentate=0, portiRallentati=0, naviAffondate=0, i;
    int pidPortoAlto=0;/*indica il pid dell'ultimo porto */
    float mortiGiornaliere=0;

    createIPCKeys();
    /*creo la sm per fare il report*/
    reportId = shmget(keyReport,sizeof(report) ,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa report");
        perror(strerror(errno));
    }

    report =shmat(reportId,NULL,0); /*specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo*/
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }

    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    portArrays = shmat(portArrayId,NULL,0);
    semDaysId=  semget(keyGiorni,SO_PORTI+SO_NAVI,0); /*creo semafori gestione giorni */
    if(semDaysId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }

    srand(getpid()/1000);
    report->affondate=0;
    report->rallentate=0;
    report->rallentati=0;
    while(portArrays[SO_PORTI-1].idPorto==0){

    } /*aspetta che si generino tutti i porti */
    sleep(0.013*SO_NAVI);
    for(i=0;i<SO_PORTI;i++){
        if(portArrays[i].idPorto>pidPortoAlto)
            pidPortoAlto=portArrays[i].idPorto;
    }

    while(giorniSimulazione<SO_DAYS && naviAffondate<SO_NAVI ){

        /*  printf("Giorno per meteo: %d.\n",giorniSimulazione);*/
        naveRallentata = (rand() %  SO_NAVI);
        if(kill(pidPortoAlto + naveRallentata + 1,SIGUSR1)==-1){
            if (errno == ESRCH) {
                printf("rallentamento nave infattibile, la nave non esiste");

            }
            else{
                TEST_ERROR;
                perror("errore durante l'invio del segnale da meteo per rallentare la nave");
            }
        }
        report->rallentate++;
        /*  printf("\n La nave %d è stata rallentata",naveRallentata);*/
        portoRallentato = (rand() %  SO_PORTI);
        if(kill(getppid() + portoRallentato + 1,SIGUSR1)==-1){
            TEST_ERROR;
            perror("errore durante l'invio del segnale da meteo per rallentare il porto");
        }
        report->rallentati++;
        /* printf("\n Il porto %d è stato rallentato",portoRallentato);*/
        /*printf("\n Il porto %d  è pidporto alto",pidPortoAlto);*/

        mortiGiornaliere = mortiGiornaliere + 24;
        if(mortiGiornaliere>SO_MAELSTROM){
            killShip=1;
            mortiGiornaliere = (int)mortiGiornaliere/SO_MAELSTROM;
        }


        for(i=0;i<mortiGiornaliere && killShip;i++) {
            naveAffondata = (rand() % SO_NAVI);

            if (kill(pidPortoAlto + naveAffondata+1, SIGTERM) == -1) {
                if (errno == ESRCH) {
                    printf("terminazione nave infattibile, la nave non esiste");
                    break;
                }else {
                    TEST_ERROR;
                    perror("errore durante l'invio del segnale da meteo per terminare la nave");
                    break;
                }
            }else{
                if (semctl(semDaysId, SO_PORTI+naveAffondata, GETVAL) < SO_DAYS) {
                    struct sembuf sops;
                    sops.sem_num = SO_PORTI+naveAffondata;
                    sops.sem_op = SO_DAYS-1;
                    sops.sem_flg = 0;
                    if(semop(semDaysId, &sops, 1)==-1){
                        TEST_ERROR
                    }
                }
                /*printf("giorno aumentato da meteo del nave %d a %d",naveAffondata,semctl(semDaysId, SO_PORTI+naveAffondata, GETVAL));*/


                report->affondate++;
            } }

        while(semctl(semDaysId,SO_PORTI-1,GETVAL) < giorniSimulazione+1){

        }
        giorniSimulazione++;

    }
    sleep(1);

}