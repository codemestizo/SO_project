#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include "utility.h"

struct stat st;
int giorniSimulazione = 0,processiMorti=0, arrayInit[17];
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }
#define BUFFERSIZE 10


void createIPCKeys(){
    keyPortArray = ftok("master.c", getpid());
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyPortArray");
    }

    keySemPortArray = ftok("nave.c", getpid());
    if(keySemPortArray == -1){
        TEST_ERROR
        perror("errore keySemPortArray");
    }
    keyMessageQueue = ftok("porto.c", getpid());
    if(keyMessageQueue == -1){
        TEST_ERROR
        perror("errore keyMessageQueue");
    }
    keySemMessageQueue = ftok("utility.c", getpid());
    if(messageQueueId == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }

    keyReport = ftok("MakeFile", getpid());
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyReport");
    }
}

void fillAndCreate_resource(){

    int i;
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * arrayInit[0])) * arrayInit[1];
    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        TEST_ERROR
    }

    portArrays =shmat(portArrayId,NULL,0); /*specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo*/
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        TEST_ERROR
    }

    /*creo la sm per fare il report*/
    reportId = shmget(keyReport,sizeof(report),IPC_CREAT | 0666);
    if(reportId == -1){
        printf("errore durante la creazione della memoria condivisa report");
        TEST_ERROR
    }

    report =shmat(reportId,NULL,0); /*specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo*/
    if (report == (void *) -1){
        printf("errore durante l'attach della memoria condivisa report durante l'avvio dell' inizializzazione");
        TEST_ERROR
    }
    semPortArrayId=  semget(keySemPortArray,arrayInit[1],IPC_CREAT | 0666); /*creo semafori della sh*/
    if(semPortArrayId == -1){
        printf("errore durante la creazione dei semafori sh");
        TEST_ERROR
    }
    for( i=0;i<arrayInit[1];i++){
        initSemAvailable(semPortArrayId,i);
    }

    semMessageQueueId=  semget(keySemMessageQueue,arrayInit[1],IPC_CREAT | 0666); /*creo semafori della coda di messaggi*/
    if(semMessageQueueId == -1){
        printf("errore durante la creazione dei semafori message");
        TEST_ERROR
    }


    messageQueueId=msgget(keyMessageQueue, IPC_CREAT | 0666); /*creo coda di messaggi*/
    if(messageQueueId == -1){
        printf("errore durante la creazione della coda messaggi");
        TEST_ERROR
    }

}


 void clean(){ /*dealloca dalla memoria*/

     int i;

     if(portArrays!=NULL){

         for(i=0;i<arrayInit[1];i++){
             if(  semctl(portArrays[i].semIdBanchinePorto,arrayInit[8],IPC_RMID)==-1){
                 if (errno == EINVAL){

                     break;
                 }

                 else
                 TEST_ERROR
             }
         }

/* 'remove' shared memory segment */
         if(shmctl(portArrayId, IPC_RMID, NULL) ==-1){
             if (errno == EINVAL)
                 printf("portarray non trovato");
             else
             TEST_ERROR
         }

         if(shmctl(reportId, IPC_RMID, NULL) ==-1){
             if (errno == EINVAL)
                 printf("semaforo report non trovato");
             else
             TEST_ERROR
         }

     }
     /* shmctl(reportId, IPC_RMID, NULL);*/
     if (msgctl(messageQueueId, IPC_RMID, NULL)== -1) { /*cancella coda di messaggi*/
         if (errno == EINVAL)
             printf("CODA MESSAGGI non trovato");
         else
         TEST_ERROR
     }

     if(semctl(semPortArrayId,arrayInit[1],IPC_RMID)==-1) {
         if (errno == EINVAL)
             printf("semaforo portArray non trovato");
         else
         TEST_ERROR
     }

     if(semctl(semMessageQueueId,arrayInit[1],IPC_RMID)==-1) {
         if (errno == EINVAL)
             printf("semaforo coda di messaggi non trovato");
         else
         TEST_ERROR
     }


 }

int main() {
     time_t endwait, actualTime;
     int best = 0, migliore = 0;
     int i, child_pid, status, l, a, fermaPorto, totalePorto;
     int bypassInit = 0;
     float timerKoNavi = 0, naviKo = 0;
     char *argv[17], *command = "", *text = malloc(sizeof(char) * BUFFERSIZE), buffer[BUFFERSIZE];
     char workString[30];


     if(!bypassInit){
         int so_porti = SO_PORTI, so_merci = SO_MERCI;
         /* ricavo dall'utente le variabili necessarie allo svolgimento della simulazione*/
         for(i=0;i<15;i++){
             switch(i){
                 case 0:
                     arrayInit[1] = so_porti;
                     arrayInit[2] = so_merci;
                     printf("inserire il numero di navi che saranno presenti nella simulazione(inserirne almeno una)\n");
                     break;
                 case 3:
                     printf("inserire il numero di giorni di vita minimi per la merce\n");
                     break;
                 case 4:
                     printf("inserire il numero di giorni di vita massimi per la merce\n");
                     break;
                 case 5:
                     printf("inserire la lunghezza del lato della mappa (quadrata)\n");
                     break;
                 case 6:
                     printf("inserire la velocità delle navi presenti nella simulazione\n");
                     break;
                 case 7:
                     printf("inserire le tonnellate che può caricare ogni nave\n");
                     break;
                 case 8:
                     printf("inserire le banchine possedute da ogni porto\n");
                     break;
                 case 9:
                     printf("inserire le tonnellate totali di merci richieste e offerte da TUTTI i porti in totale\n");
                     break;
                 case 10:
                     printf("inserire la velocità di carico delle navi\n");
                     break;
                 case 11:
                     printf("inserire il numero di giorni(secondi) in cui si protrarrà la simulazione\n");
                     break;
                 case 12:
                     printf("inserire il numero di ore per cui una nave viene rallentata\n");
                     break;
                 case 13:
                     printf("inserire il numero di ore per cui un porto viene rallentato\n");
                     break;
                 case 14:
                     printf("inserire il numero di ore ogni quanto affonda una nave\n");
                     break;
                 default:
                     break;
             }
             if(i!=1 && i!=2){
                 fgets(buffer, BUFFERSIZE , stdin);
                 strcpy( text, buffer );
                 if(isdigit(*text)==0){
                     printf("insert a number");
                     i--;
                 }
                 else{
                     arrayInit[i] = atoi(text);
                     if(arrayInit[0]<1){
                         arrayInit[0] = 0;
                         i--;
                         printf("insert a bigger value");
                     }
                 }
             }

         }

         arrayInit[15] = arrayInit[9]/arrayInit[10];/*SO_SIZE*/
     }
     else{
         arrayInit[0] = SO_NAVI;
         arrayInit[1] = SO_PORTI;
         arrayInit[2] = SO_MERCI;
         arrayInit[3] = SO_MIN_VITA;
         arrayInit[4] = SO_MAX_VITA;
         arrayInit[5] = SO_LATO;
         arrayInit[6] = SO_SPEED;
         arrayInit[7] = SO_CAPACITY;
         arrayInit[8] = SO_BANCHINE;
         arrayInit[9] = SO_FILL;
         arrayInit[10] = SO_LOADSPEED;
         arrayInit[11] = SO_DAYS;
         arrayInit[12] = SO_STORM_DURATION;
         arrayInit[13] = SO_SWELL_DURATION;
         arrayInit[14] = SO_MAELSTROM;
         arrayInit[15] = SO_FILL/SO_PORTI;

     }

    for(i=0;i<16;i++){
        printf("cellaArray[%d]: %d\n", i,arrayInit[i]);
    }
     clean();

     for(i=0;i<16;i++){
         sprintf(workString, "%d", arrayInit[i]);
         argv[i] = strdup(workString);
     }
     argv[16] = NULL;
     createIPCKeys();
     fillAndCreate_resource(); /* istanzia tutte le varie code,semafori,memorie condivise necessarie PER TUTTI i processi(keyword static)*/
     /*clean();*/
     sleep(1);
     /*fillAndCreate_resource();*/

     printf("messageQueueIdInizio: %d\n, semPortArrayIdInizio: %d\n, semMessageQueueIdInizio: %d\n",messageQueueId,semPortArrayId,semMessageQueueId);
     /*creazione processi porto*/
    for (i = 0; i < arrayInit[1]; i++) {
        sleep((unsigned int) 0.60);
        switch (fork()) {
            case 0:
                /* Handle error */

                command = "./porto";
                if (execvp(command, argv) == -1) {
                    printf("errore durante l'esecuzione del execvp per il porto \n");
                    TEST_ERROR
                }
                exit(EXIT_FAILURE);
            case -1:

                break;
            default:
                break;
        }

    }
    sleep(arrayInit[0] * 0.55);
    for (i = 0; i < arrayInit[0]; i++) {
        sleep((unsigned int) 0.01);
        switch (fork()) {
            case 0:
                /* Handle error */

                command = "./nave";
                if (execvp(command, argv) == -1) {
                    printf("errore durante l'esecuzione del execvp per la nave \n");
                    TEST_ERROR
                }
                exit(EXIT_FAILURE);
            case -1:


                exit(EXIT_FAILURE);
                break;

            default:
                break;
        }

    }

    switch (fork()) {
        case 0:
            /* Handle error */

            command = "./meteo";
            if (execvp(command, argv) == -1) {
                printf("errore durante l'esecuzione del execvp per il processo meteo \n");
                TEST_ERROR
            }
            exit(EXIT_FAILURE);

        case -1:
            /*padre*/
            exit(EXIT_FAILURE);
            break;

        default:
            break;
    }

     endwait = time (NULL) + arrayInit[11];
     actualTime = time(NULL);
     if(arrayInit[14]>0){
         naviKo = (float) 24/(float)arrayInit[14]; /*numero di navi morte al giorno*/
     }
     else{
         naviKo = 0;
     }

     printf("naviKO %f\n", naviKo);
     while (time (NULL) < endwait){
         int stopSystem = 0;
         /*ogni giorno invio il segnale di incremento*/
         if((time(NULL)-1)>=actualTime){
            actualTime = time(NULL);
             i=0;
            for(i=1;i<=arrayInit[1] + arrayInit[0] + 2;i++){
                if(kill((getpid() + i), SIGUSR2)==-1){
                    if(errno==EEXIST){
                        printf("nave non presente, system call kill failed");
                    }
                }
            }
            timerKoNavi += naviKo;
             printf("timerKoNavi %f\n", timerKoNavi);
            /*printf("timerKoNavi: %f \n, naviKo: %f", timerKoNavi, naviKo);*/
            if(timerKoNavi >= (float)arrayInit[0])
                stopSystem = 1;
         }
         if(stopSystem){
            for(i=1;i<=arrayInit[1] + arrayInit[0] + 2;i++){
                kill((getpid() + i), SIGTERM);
                printf("affondata ogni nave, terminazione anticipata della simulazione da master.c %d, indice i: %d\n",getpid() + i, i);
                endwait = time(NULL);
            }
         }
     }

    /* appena un figlio termina eseguo altre operazioni */
    while ((child_pid = wait(&status)) != -1) {
        printf("PARENT: PID=%d. Got info of child with PID=%d, status=0x%04X\n", getpid(), child_pid, status);
        processiMorti++;
    }

    if(processiMorti==(arrayInit[0]+arrayInit[1]+1)){
    printf("\nSIMULAZIONE TERMINATA\n");

    printf("\n\n<==============================>\n Durante la simulazione sono state:\n Rallentate %d Navi\n Rallentati %d Porti\n Affondate %d Navi\n<==============================>\n\n",
           report->rallentate, report->rallentati, report->affondate);


    printf("\n\nCi sono %d navi in mare senza carico\nCi sono %d navi in mare con carico\nCi sono %d navi a commerciare ai porti\n",
           report->senzaCarico, report->conCarico, report->inPorto);

    for (l = 0; l < arrayInit[1]; l++) {
        totalePorto = 0;
        for (a = 0; a < arrayInit[2]; a++) {
            if (portArrays[l].merce[a].offertaDomanda == 1)
                totalePorto += portArrays[l].merce[a].quantita;
        }
        printf("\n\n Nel porto %d sono:\n -presenti %d tonnellate di merce\n -state ricevute %d tonnellate di merce\n -state spedite %d tonnellate di merce\n",
               l, totalePorto, report->ricevutePorto[l], report->speditePorto[l]);
    }


    for (l = 0; l < arrayInit[2]; l++) {
        fermaPorto = 0;
        for (a = 0; a < arrayInit[1]; a++) {
            if (portArrays[a].merce[l].offertaDomanda == 1)
                fermaPorto += portArrays[a].merce[l].quantita;
        }
        printf("\n\nLa merce %d è:\n -stata generata all'inizio una quantità pari a %d tonnellate\n -rimasta ferma nei porti in quantità pari a %d (se maggiore è causata dall'arrivo in porto di nuova merce non per via marittima)\n -scaduta nei porti in quantità pari a %d\n -scaduta nelle navi in quantità pari a %d\n -consegnata da qualche nave in quantità pari a %d tonnellate \n",
               l, report->merciGenerate[l], fermaPorto, report->merciScadutePorto[l], report->merciScaduteNave[l],
               report->consegnataDaNave[l]);
    }


    for (l = 0; l < arrayInit[1]; l++) {
        if (report->offerte[l] > best) {
            best = report->offerte[l];
            migliore = l;
        }
    }
    printf("\nIl porto con la miglior offerta e' stato %d\n", migliore);

    printf("\nIl porto con la maggior richiesta è stato %d", report->migliorRichiesta);

    clean();

    }
}
