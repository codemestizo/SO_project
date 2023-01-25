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


    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    portArrays = shmat(portArrayId,NULL,0);
    semDaysId=  semget(keyGiorni,SO_PORTI+SO_NAVI,0); //creo semafori gestione giorni
    if(semDaysId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }

    int killShip=0;
    int giorniSimulazione=0;
    int numeroNavi=0;
    int numeroPorti=0;
    float mortiGiornaliere=0;
    int naveRallentata=0;
    int portoRallentato=0;
    int naveAffondata=0;
    int pidPortoAlto=0;//indica il pid dell'ultimo porto
    int naviRallentate=0;
    int portiRallentati=0;
    int naviAffondate=0;
    srand(getpid()/1000);
    while(portArrays[SO_PORTI-1].idPorto==0){

    } //aspetta che si generino tutti i porti
     sleep(0.013*SO_NAVI);
    for(int i=0;i<SO_PORTI;i++){
        if(portArrays[i].idPorto>pidPortoAlto)
            pidPortoAlto=portArrays[i].idPorto;
    }


    while(giorniSimulazione<SO_DAYS && naviAffondate<SO_NAVI ){

        printf("Giorno per meteo: %d.\n",giorniSimulazione);
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
        naviRallentate++;
        printf("\n La nave %d è stata rallentata",naveRallentata);
        portoRallentato = (rand() %  SO_PORTI);
        if(kill(getppid() + portoRallentato + 1,SIGUSR1)==-1){
            TEST_ERROR;
            perror("errore durante l'invio del segnale da meteo per rallentare il porto");
        }
        portiRallentati++;
        printf("\n Il porto %d è stato rallentato",portoRallentato);
        printf("\n Il porto %d  è pidporto alto",pidPortoAlto);

        mortiGiornaliere = mortiGiornaliere + 24;
        if(mortiGiornaliere>SO_MAELSTROM){
            killShip=1;
            mortiGiornaliere = (int)mortiGiornaliere/SO_MAELSTROM;
        }


        for(int i=0;i<mortiGiornaliere && killShip;i++) {
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
                    /*if (releaseSem(semDaysId, SO_PORTI+naveAffondata) == -1) {
                        printf("errore durante l'incremento del semaforo per incrementare i giorni ");
                        TEST_ERROR;
                    }*/
                    struct sembuf sops;
                    sops.sem_num = SO_PORTI+naveAffondata;
                    sops.sem_op = SO_DAYS-1;
                    sops.sem_flg = 0;
                    if(semop(semDaysId, &sops, 1)==-1){
                        TEST_ERROR
                    }
                }
                printf("giorno aumentato da meteo del nave %d a %d",naveAffondata,semctl(semDaysId, SO_PORTI+naveAffondata, GETVAL));


            printf("\n Affondati la nave %d",naveAffondata);
            naviAffondate++;
            //break;
        } }



        while(semctl(semDaysId,SO_PORTI-1,GETVAL) < giorniSimulazione+1){

        }
          giorniSimulazione++;

    }
    sleep(0.5);
    printf("\n\n<==============================>\n Durante la simulazione sono state:\n Rallentate %d Navi\n Rallentati %d Porti\n Affondate %d Navi\n<==============================>\n\n",naviRallentate,portiRallentati,naviAffondate);


}