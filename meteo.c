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
#include <time.h>

#include "utility.h"

#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }

int giorniSimulazione=0;

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


    keyReport = ftok("master.c", 'r');
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyPortArray");
    }
}

void handle_signal(int signum) {
    struct timespec tim, tim2;
    char str[10];
    tim.tv_sec = 0;
    if(SO_SWELL_DURATION<10)
        sprintf(str,"%d",SO_SWELL_DURATION*10);
    else
        sprintf(str,"%d",SO_STORM_DURATION);
    strcat(str,"000000L");
    tim.tv_nsec = atoi(str);
    switch (signum) {

        case SIGUSR1:
            nanosleep(&tim,&tim2);
            break;
        case SIGUSR2:
            printf("incremento giorno simulazione dentro meteo.c");
            giorniSimulazione++;
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    int killShip=0, giornoAttuale=0, naveRallentata=0, portoRallentato=0, naveAffondata=0, naviRallentate=0, portiRallentati=0, naviAffondate=0, i,mortiGiornaliere=0;
    int pidPortoAlto=0;/*indica il pid dell'ultimo porto */
    struct sigaction sa;
    sigset_t my_mask;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_signal;
    sigemptyset(&my_mask); /* do not mask any signal */
    sa.sa_mask = my_mask;
    sigaction(SIGUSR2, &sa, 0);


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
    //TODO capire perchè meteo non termina/non affonda nessuna nave
    while(giorniSimulazione<SO_DAYS && report->affondate!=SO_NAVI ){
        sigaction(SIGUSR2, &sa, 0);

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

        portoRallentato = (rand() %  SO_PORTI);
        if(kill(getppid() + portoRallentato + 1,SIGUSR1)==-1){
            if (errno == ESRCH) {
                printf("rallentamento porto infattibile, il porto non esiste");

            }
            else{
                TEST_ERROR;
                perror("errore durante l'invio del segnale da meteo per rallentare il porto");
            }
        }
        report->rallentati++;

        mortiGiornaliere = mortiGiornaliere + 24;
        if(mortiGiornaliere>SO_MAELSTROM){
            killShip = mortiGiornaliere/SO_MAELSTROM; /*navi da terminare in questa giornata*/
            mortiGiornaliere = mortiGiornaliere%SO_MAELSTROM; /*tengo conto delle ore rimanenti per terminare la corretta quantità di navi*/
        }


        for(i=0;i<killShip && report->affondate<SO_NAVI ;i++ ) {
            naveAffondata = (rand() % SO_NAVI);

            if (kill(pidPortoAlto + naveAffondata+1, SIGTERM) == -1) {
                if (errno == ESRCH) {
                    printf("\nIl maelstorm si è abbattuto su una nave già affondata");

                    break;
                }else {
                    TEST_ERROR;
                    perror("errore durante l'invio del segnale da meteo per terminare la nave");
                    break;
                }
            }else{
                report->affondate++;
            }
        }
        killShip=0;

        /*sigemptyset (&my_mask);
        sigfillset(&my_mask);
        sigdelset(&my_mask, SIGUSR2);*/
        while (giornoAttuale == giorniSimulazione)
            sigsuspend (NULL);
        giornoAttuale = giorniSimulazione;

    }
    sleep(1);

}