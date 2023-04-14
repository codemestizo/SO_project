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

int giorniSimulazione=1;
int so_navi=0,so_porto=0,so_merci=0,so_min_vita=0,
        so_max_vita=0,so_lato=0,so_speed=0,so_capacity=0,
        so_banchine=0,so_fill=0,so_loadspeed=0,so_days=0,
        so_storm_duration=0,so_swell_duration=0,so_maelstrom=0;

void createIPCKeys() {
    keyPortArray = ftok("master.c", getppid());
    if (keyPortArray == -1) {
        TEST_ERROR
        perror("errore keyPortArray");
    }

    keySemPortArray = ftok("nave.c", getppid());
    if (keySemPortArray == -1) {
        TEST_ERROR
        perror("errore keySemPortArray");
    }
    keyMessageQueue = ftok("porto.c", getppid());
    if (keyMessageQueue == -1) {
        TEST_ERROR
        perror("errore keyMessageQueue");
    }
    keySemMessageQueue = ftok("utility.c", getppid());
    if (messageQueueId == -1) {
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }

    keyReport = ftok("MakeFile", getppid());
    if (keyPortArray == -1) {
        TEST_ERROR
        perror("errore keyReport");
    }
}

void handle_signal(int signum) {
    switch (signum) {
        case SIGUSR2:
            giorniSimulazione++;
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    int size = 0;
    int killShip=0, giornoAttuale=0, naveRallentata=0, portoRallentato=0, naveAffondata=0, naviRallentate=0, portiRallentati=0, naviAffondate=0, i,mortiGiornaliere=0;
    int pidPortoAlto=0;/*indica il pid dell'ultimo porto */
    struct sigaction sa;
    sigset_t my_mask;

    so_navi=atoi(argv[0]),so_porto=atoi(argv[1]),so_merci=atoi(argv[2]),so_min_vita=atoi(argv[3]),
    so_max_vita=atoi(argv[4]),so_lato=atoi(argv[5]),so_speed=atoi(argv[6]),so_capacity=atoi(argv[7]),
    so_banchine=atoi(argv[8]),so_fill=atoi(argv[9]),so_loadspeed=atoi(argv[10]),so_days=atoi(argv[11]),
    so_storm_duration=atoi(argv[12]),so_swell_duration=atoi(argv[13]),so_maelstrom=atoi(argv[14]);
    size = (sizeof(portDefinition) + (sizeof(structMerce) * so_merci)) * SO_PORTI;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_signal;
    sigemptyset(&my_mask); /* do not mask any signal */
    sa.sa_mask = my_mask;
    sa.sa_flags = SA_NODEFER;
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

    srand(getpid()/1000);
    report->affondate=0;
    report->rallentate=0;
    report->rallentati=0;

    while(portArrays[SO_PORTI-1].idPorto==0){

    } /*aspetta che si generino tutti i porti */

    sleep(0.013*so_navi);
    for(i=0;i<SO_PORTI;i++){
        if(portArrays[i].idPorto>pidPortoAlto)
            pidPortoAlto=portArrays[i].idPorto;
    }
    while(giorniSimulazione<so_days && report->affondate!=so_navi ){
        sigaction(SIGUSR2, &sa, 0);

        naveRallentata = (rand() %  so_navi);
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
        if(mortiGiornaliere>so_maelstrom){
            killShip = mortiGiornaliere/so_maelstrom; /*navi da terminare in questa giornata*/
            mortiGiornaliere = mortiGiornaliere%so_maelstrom; /*tengo conto delle ore rimanenti per terminare la corretta quantità di navi*/
        }


        for(i=0;i<killShip && report->affondate<so_navi ;i++ ) {
            naveAffondata = (rand() % so_navi);

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

        sigemptyset (&my_mask);
        sigfillset(&my_mask);
        sigdelset(&my_mask, SIGUSR2);
        while (giornoAttuale == giorniSimulazione && giorniSimulazione<so_days)
            sigsuspend (&my_mask);
        giornoAttuale = giorniSimulazione;

    }
    sleep(1);

}