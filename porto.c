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


// il semaforo è semPortArrayId id , devo poi chiamare la funzione reserve semaphore e gli passo portArrayid a reserve sem e secondo parametro 1
//devo fare release sem prima perchè mi torna la semop che prova a decrementare il semaforo di 1
//dopo aver fatto l'inserimento chiamo release semaphore che aumenta il valore di 1
//funzione che riempirà le struct dei porti

void setPorto(portDefinition *portArrays){
    /*if(reserveSem( semPortArrayId, 1)==-1){ //richiede la memoria e la occupa SOLO LUI
        printf("errore durante il decremento del semaforo per inizializzare il porto");
        perror(strerror(errno));
    }*/
    //printf("sono in porto");


    int i=0;
    while(portArrays[i].idPorto!=0){
        i++;
    }
    if(portArrays[i].idPorto==0){
        portArrays[i].idPorto=getpid();
        //portArrays[i].semIdBanchinePorto = semget(IPC_PRIVATE,SO_BANCHINE,0600);

        if(i==0){ //set spawn porto
            portArrays[i].x=0;
            portArrays[i].y=0;
        }else if(i==1){
            portArrays[i].x=SO_LATO;
            portArrays[i].y=0;
        }else if(i==2){
            portArrays[i].x=SO_LATO;
            portArrays[i].y=SO_LATO;
        }else if(i==3){
            portArrays[i].x=0;
            portArrays[i].y=SO_LATO;
        }else {
            srand(time(NULL));
            portArrays[i].x=(rand() %  (int)SO_LATO+1);
            portArrays[i].y=(rand() %  (int)SO_LATO+1);
            for(int j=0;j<i-1;j++){ //controllo che non spawni sulla posizione di un altro porto
                if(portArrays[i].x== portArrays[j].x && portArrays[i].y==portArrays[j].y){
                    j=-1;
                    portArrays[i].x=(rand() %  (int)SO_LATO+1);
                    portArrays[i].y=(rand() %  (int)SO_LATO+1);
                }

            }
        }

    }
    for(int k=0;k<SO_MERCI;k++){
        srand(time(NULL));
        portArrays[i].merce[k].nomeMerce = k;
        portArrays[i].merce[k].offertaDomanda = (rand() %  2);//0 = domanda, 1 = offerta, 2 = da assegnare
        if(portArrays[i].merce[k].offertaDomanda ==1)
            portArrays[i].merce[k].vitaMerce = (SO_MIN_VITA + (rand() %  (SO_MAX_VITA-SO_MIN_VITA))); //giorni di vita

    }
    /*if(releaseSem(semPortArrayId, 1)==-1){
        printf("errore durante l'incremento del semaforo dopo aver inizializzato il porto");
        perror(strerror(errno));
    }*/


}


void gestioneInvecchiamentoMerci(portDefinition *portArrays){ //funzione da richiamare ogni "giorno" di simulazione per checkare se la merce del porto è scaduta
    for(int i=0;i<SO_PORTI;i++){
        for(int k=0;k<SO_MERCI;k++){
            if(portArrays[i].merce[k].vitaMerce <=0){ //decidere se cancellare proprio o settare a 0 e da assegnare il tutto
                portArrays[i].merce[k].offertaDomanda=2;
                portArrays[i].merce[k].vitaMerce=0;
            }
            else{
                portArrays[i].merce[k].vitaMerce-=1;
            }
        }
    }
}

int stampaStatoMemoriaa() {
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

int startPorto(int argc, char *argv[]){
    printf(" PID DI STO PROCESSO %d",getpid());
    printf("keyPortArray %d \n",keyPortArray);

    //keyPortArray = ftok("master.c", 'u');
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(keyPortArray,size,0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        TEST_ERROR
    }
    portArrays = shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        TEST_ERROR
    }

    stampaStatoMemoriaa();
    setPorto( portArrays);
    for(int i = 0;i<SO_PORTI;i++){

        printf("X del porto %d: %d   \n",i,portArrays[i].x);
        printf("Y del porto %d: %d   \n",i,portArrays[i].y);
        printf("ID DEL PORTO :%d \n",portArrays[i].idPorto);
        /* for(int j=0;j<SO_MERCI;j++){
            printf("La merce numero %d e' richieta/offerta/non (%d)  in qualita' pari a :%f tonnellate con una vita (se venduta)  di %d giorni \n",portArrays[i].merce[j].nomeMerce,portArrays[i].merce[j].offertaDomanda,portArrays[i].merce[j].quantita,portArrays[i].merce[j].vitaMerce);
         */}


    portArrays[0].merce[1].offertaDomanda=1;
    portArrays[0].merce[1].quantita=10;
    portArrays[0].merce[1].vitaMerce=40;

    if ((messageQueueId = msgget(keyMessageQueue, 0644)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }
    printf("message queue: ready to receive messages.\n");


    while(SO_DAYS-giorniSimulazione>0){

        if (msgrcv(messageQueueId, &buf, 100, 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        int merceSpostata=portArrays[0].merce[buf.nomeMerce].quantita; //temporanea per salvarmi quanta merda togliere o far rimanere
            (portArrays[0].merce[buf.nomeMerce].quantita )- buf.quantita;
        if(portArrays[0].merce[buf.nomeMerce].quantita<0) { //se non basta, al massimo tornerà con tutta la merce che c'era in porto
            portArrays[0].merce[buf.nomeMerce].quantita = 0;
            buf.quantita=merceSpostata;
        }
        portArrays[0].merce[buf.nomeMerce].offertaDomanda=2; //non c'è piu quindi da assegnare

        if((msgsnd(messageQueueId,&buf,100,0))==-1){
            printf("Errore mentre faceva il messaggio");
            perror(strerror(errno));
        }else printf("Risposta mandata");

        if (msgctl(messageQueueId, IPC_RMID, NULL) == -1) {
            perror("msgctl");
            exit(1);
        }
        printf("Giorno: %d.\n",giorniSimulazione);
    giorniSimulazione++;
        sleep(1);
    }

    return 0;
}






