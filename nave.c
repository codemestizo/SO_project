#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <bits/types/struct_timespec.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <bits/types/sigset_t.h>
#include <bits/sigaction.h>
#include "utility.h"

#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }
/* Processo nave */
int comunicato=0;
int controllato=0;
int xNave = 0;
int yNave = 0;
int giorniSimulazioneNave = 0;
int numeroNave;
int com=0;
int merceScaduta=0;
float residuoCapacitaNave = SO_CAPACITY;
int xPortoMigliore=-1, yPortoMigliore=-1;
int statoNave=0; /*0 in mare senza carico, 1 in mare con carico, 2 sta in porto a comprare/vendere */
structMerce *merciNave; /* puntatore all'array delle merci della nave */
int pidPortoDestinazione, idSemBanchine;


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
        perror("errore keySemMessageQueueId");
    }
    keyGiorni = ftok("master.c", 'o');
    if(keyGiorni == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }


    keyReport = ftok("master.c", 'r');
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyPortArray");
    }
}

void searchPort( ) {/*array porti, array di merci della nave */
    int i,k, valoreMerceMassimo = 0, banchinaLibera = 0; /*coefficenteDistanza = distanza tra porti/merce massima, utilizzato per valutare la bontà della soluzione */
    float coefficente = 0, xAux = 0, yAux = 0;
    float attualeMigliore=0;
    int distanza=0;
    float vita=0;
    float occupato=0;
    float parificatore=0;/*serve per il coefficiente (per veder ESCLUSIVAMENTE la merce scambiata quando ne avanza da porto/nave), per renderlo più efficiente */
    for (i = 0; i < SO_PORTI; i++) {
        occupato=0;
        coefficente=0;
        distanza=(xNave+yNave)-(portArrays[i].x+portArrays[i].y);
        if(distanza<0)
            distanza=distanza*(-1);
        for (k = 0; k < SO_MERCI; k++) {/*0 = domanda, 1 = offerta, 2 = da assegnare */
            if (merciNave[k].offertaDomanda==1 && portArrays[i].merce[k].offertaDomanda==0) { /*vedo se il porto vuole la merce */
                vita= merciNave[k].vitaMerce - (distanza/ SO_SPEED);

                if(merciNave[k].quantita<portArrays[i].merce[k].quantita){ /*controllo che la merce sulla nave da vendere sia + o - di quella richiesta */
                    parificatore=0;                                        /* ovviamente se la nave ha più della merce richiesta, il coefficiente sarà minore in quanto sarà spostata meno merce */
                }else if(merciNave[k].quantita>portArrays[i].merce[k].quantita)
                    parificatore=merciNave[k].quantita-portArrays[i].merce[k].quantita;

                if(vita>0) {
                    coefficente += ((merciNave[k].quantita-parificatore) / distanza);
                    occupato-=merciNave[k].quantita;
                }

            }else  if (merciNave[k].offertaDomanda==0 && portArrays[i].merce[k].offertaDomanda==1) {/*vedo se il porto propone la merce */
                vita= portArrays[i].merce[k].vitaMerce - (distanza/ SO_SPEED);

                if(merciNave[k].quantita<portArrays[i].merce[k].quantita){ /*controllo che la merce nel porto da vendere sia + o - di quella richiesta */
                    parificatore=0;                                        /* ovviamente se il porto avesse più della merce richiesta, il coefficiente sarà minore in quanto sarà spostata meno merce */
                }else if(merciNave[k].quantita>portArrays[i].merce[k].quantita)
                    parificatore=merciNave[k].quantita-portArrays[i].merce[k].quantita;



                if(vita>0 && occupato+portArrays[i].merce[k].quantita<SO_CAPACITY) {
                    coefficente += ((merciNave[k].quantita -parificatore)/ distanza);
                    occupato+=merciNave[k].quantita; /*quantità che prenderebbe la nave */
                }

            }
        }

        if(coefficente>attualeMigliore){
            attualeMigliore=coefficente;
            xPortoMigliore=portArrays[i].x;
            yPortoMigliore=portArrays[i].y;
            idSemBanchine = portArrays[i].semIdBanchinePorto;
        }


    }

}




/*funzione che valorizza le informazioni per una singola struct dell'array di struct delle merci */
/*si assume che l'indice della cella dell'array corrisponda al nome della merce, sennò l'interpretazione non funziona */
void interpretaSezioneMessaggio(const char sezioneMessaggio[], int indiceMerce){
    int i;
    int check = 1;

    for(i = 0;i < strlen(sezioneMessaggio) && check != 0;i++){
        int commaCounter = 0;
        char c = sezioneMessaggio[i];
        char value[10] = " ";
        if(c != ';')
            strcat(value,&c);
        else{
            commaCounter++;
            if(commaCounter == 1 && merciNave[indiceMerce].offertaDomanda != atoi(value)){
                check = 0;
                strcpy(value," ");
            }else if(commaCounter == 1 && merciNave[indiceMerce].offertaDomanda == atoi(value)){
                strcpy(value," ");
            }else if(commaCounter == 2){
                merciNave[indiceMerce].quantita = merciNave[indiceMerce].quantita - atoi(value);
                if(merciNave[indiceMerce].quantita < 0)
                    merciNave[indiceMerce].quantita = 0;
                strcpy(value," ");
            }
            else if(commaCounter == 3){
                merciNave[indiceMerce].vitaMerce = atoi(value);
                strcpy(value," ");
            }

        }
    }

}

int findNumSem(){
    int numSem,i;
    for(i=0;i<SO_BANCHINE;i++){
        numSem = semctl(idSemBanchine,i,GETVAL);
        if(numSem == 0)
            return i;
    }
    return -1;
}

void comunicazionePorto() {
    struct messagebuf buf;
    struct messagebuf buf1;
    struct timespec tim,tim2;
    int i,banchinaRitorno,banchinaPiena=0,numSemBanchina,jump,scadenza = 0,quantitaAttuale = 0,nomeMerceChiesta = 0,pidPort = 0,scadenzaAttuale = 0,sep = 0,sommaMerciScambiate=0;
    int ron = 2;/*richiesta offerta non */
    char messaggio[30 * SO_MERCI], workString[40], delim[] = "|", tmp[20],str[10];
    char *ptr = strtok(buf1.mText, delim);
    statoNave=2;

    for (i = 0; i < SO_PORTI; i++) {
        if (xNave == portArrays[i].x && yNave == portArrays[i].y)
            pidPortoDestinazione = portArrays[i].idPorto;
    }

    buf.mType = pidPortoDestinazione;

    /*for di creazione messaggio per il porto desiderato */
    strcpy(workString, "");
    strcpy(messaggio, "");
    for (i = 0; i < SO_MERCI; i++) {

        sprintf(workString, "%d|%d|%d|%d|", getpid(), merciNave[i].nomeMerce, merciNave[i].offertaDomanda,
                merciNave[i].quantita);
        strcat(messaggio, workString);
        strcpy(workString, "");
        sommaMerciScambiate+= merciNave[i].quantita;

    }
    strcpy(buf.mText, messaggio);

    numSemBanchina = findNumSem();
    if (numSemBanchina == -1)
        banchinaPiena=1;
    if(banchinaPiena!=1) {
        if (releaseSem(idSemBanchine, numSemBanchina) == -1) {
            printf("errore durante l'incremento del semaforo per scrivere sulla coda di messaggi in nave.c");
            TEST_ERROR;
        }
        if ((msgsnd(messageQueueId, &buf, sizeof(buf.mText), 0)) == -1) {
            printf("Errore mentre faceva il messaggio");
            TEST_ERROR;
        } else {
            printf("messaggio inviato, inizio comunicazione, nave.c\n");
            banchinaRitorno = idSemBanchine;

            /*trattengo la banchina per sommaMerciScambiate/SO_LOADSPEED*/
            tim.tv_sec = (int) sommaMerciScambiate/SO_LOADSPEED;
            sprintf(str,"%d",(sommaMerciScambiate/SO_LOADSPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
            sommaMerciScambiate=0;
        }

        waitSem(idSemBanchine, numSemBanchina);

        jump=0;/*salta il check del messaggio se non riceve cosa desiderato */
        /* il semaforo va a 0 */
        if (msgrcv(messageQueueId, &buf1, sizeof(buf1.mText), getpid(), IPC_NOWAIT) == -1) {
            if(errno==ENOMSG){
                printf("non è stato trovato il messaggio richiesto, perso un messaggio in una banchina,nave.c\n");
            }
            jump=1;
        } else {
            printf("messaggio ricevuto, lettura e fine comunicazione, nave.c\n");
        }
        numSemBanchina = 0;
        idSemBanchine = 0;
        initSemAvailable(idSemBanchine, numSemBanchina);


        if(jump!=1){
            /*vado a decifrare il messaggio e settare nave */
            sep++;
            while (ptr != NULL) {
                if (sep == 1) {
                    strcpy(tmp, ptr);
                    pidPort = atoi(tmp);
                } else if (sep == 2) {
                    strcpy(tmp, ptr);
                    nomeMerceChiesta = atoi(tmp);
                } else if (sep == 3) {
                    strcpy(tmp, ptr);
                    ron = atoi(tmp);
                    if (ron == 0){
                        merciNave[nomeMerceChiesta].offertaDomanda = ron;
                    }
                    else if (ron == 1){
                        merciNave[nomeMerceChiesta].offertaDomanda = ron;
                    }


                } else if (sep == 4) {
                    strcpy(tmp, ptr);
                    quantitaAttuale = atoi(tmp);
                } else if (sep == 5) {
                    strcpy(tmp, ptr);
                    scadenzaAttuale = atoi(tmp);
                    if (ron == 2)
                        scadenzaAttuale = 0;
                    merciNave[nomeMerceChiesta].offertaDomanda = ron;
                    merciNave[nomeMerceChiesta].quantita = quantitaAttuale;
                    merciNave[nomeMerceChiesta].vitaMerce = scadenzaAttuale;
                    sep = 0;
                }
                ptr = strtok(NULL, delim);
                sep++;
                statoNave=1;
            }
        }numSemBanchina = 0;
        idSemBanchine = 0;
        initSemAvailable(idSemBanchine, numSemBanchina);
    }
    comunicato = 0;

    controllato = 0; /*se ne va a cercare un altro porto */
    srand(getpid());
    xNave=(rand() %  SO_LATO);
    yNave=(rand() %  SO_LATO); /*faccio allontanare la nave casualmente */

}

void movimento(){
    int k;
    struct timespec tim, tim2;

    for(k=0;k<SO_MERCI;k++){
        if(merciNave[k].offertaDomanda==1)
            statoNave=1;
    }
    if(xNave!=xPortoMigliore || yNave!= yPortoMigliore){

        if(xNave < xPortoMigliore && yNave < yPortoMigliore){
            char str[10];
            tim.tv_sec = (int) (((xPortoMigliore - xNave) + (yPortoMigliore - yNave))/SO_SPEED);
            sprintf(str,"%d",(((xPortoMigliore - xNave) + (yPortoMigliore - yNave))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
        }else if(xNave > xPortoMigliore && yNave < yPortoMigliore){
            char str[10];
            tim.tv_sec = (int) (((xNave-xPortoMigliore) + (yPortoMigliore - yNave))/SO_SPEED);
            sprintf(str,"%d",(((xNave - xPortoMigliore) + (yPortoMigliore - yNave))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
        }else if(xNave < xPortoMigliore && yNave > yPortoMigliore){
            char str[10];
            tim.tv_sec = (int) (((xPortoMigliore-xNave) + (yNave - yPortoMigliore))/SO_SPEED);
            sprintf(str,"%d",(((xPortoMigliore - xNave) + (yNave - yPortoMigliore))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
        }else if(xNave > xPortoMigliore && yNave > yPortoMigliore){
            char str[10];
            tim.tv_sec = (int) (((xNave-xPortoMigliore) + (yNave - yPortoMigliore))/SO_SPEED);
            sprintf(str,"%d",(((xNave-xPortoMigliore) + (yNave - yPortoMigliore))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
        }

        xNave = xPortoMigliore;
        yNave = yPortoMigliore;

    }else if(xNave==xPortoMigliore && yNave== yPortoMigliore && comunicato<1){

        comunicato+=1;
        comunicazionePorto();

    }

}


void gestioneInvecchiamentoMerci(){ /*funzione da richiamare ogni "giorno" di simulazione per controllare se la merce del porto è scaduta */
    int k=0;
    for( k=0;k<SO_MERCI;k++){
        if(merciNave[k].offertaDomanda==1){
            if(merciNave[k].vitaMerce <=0 && merciNave[k].offertaDomanda==1){ /*decidere se cancellare o inizializzare */
                merciNave[k].offertaDomanda=2;
                merciNave[k].vitaMerce =0;
                merceScaduta+= merciNave[k].quantita;
                report->merciScaduteNave[k]+=merciNave[k].quantita;
                /*scadute[k]+= portArrays[indicePorto].merce[k].quantita;*/
                merciNave[k].quantita=0;
            }
            else{
                merciNave[k].vitaMerce-=1;
            }
        }
    }
}

void generaNave(){
    int i,utile=0;
    srand(getpid());
    xNave=(rand() %  SO_LATO);
    yNave=(rand() %  SO_LATO);

    while(utile<SO_MERCI/2){ /*ciò mi permette  che la nave nasca con almeno delle richieste, e che non sia inutile la sua creazione(in caso sarebbero risorse cpu sprecate) */
        for(i=0;i<SO_MERCI;i++){
            merciNave[i].vitaMerce = 0;
            merciNave[i].nomeMerce = i;
            merciNave[i].offertaDomanda = (rand() %  3);/*0 = domanda, 1 = offerta, 2 = da assegnare */
            merciNave[i].quantita = (float)(rand() % ((int)SO_CAPACITY/SO_MERCI));
            if(merciNave[i].offertaDomanda !=0)
            {
                merciNave[i].quantita = 0.0;
                merciNave[i].offertaDomanda =2;
            }
            if(merciNave[i].offertaDomanda !=2)
                utile+=1;
        }
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
            giorniSimulazioneNave++;
            break;
        case SIGTERM:
            clean();
            kill(getpid(),SIGTERM);
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {

    int i,j,pidPortoAlto=0,giornoAttuale=0;
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    char out[100];
    struct sigaction sa;
    sigset_t my_mask;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_signal;
    sigemptyset(&my_mask); /* non chiede nessun segnale*/
    sa.sa_mask = my_mask;
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    createIPCKeys();
    messageQueueId=msgget(keyMessageQueue, IPC_CREAT | 0666); /*ottengo la coda di messaggi */
    merciNave = malloc(sizeof(structMerce) * SO_MERCI);
    generaNave();


    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);

    portArrays = shmat(portArrayId,NULL,0);
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray nel processo nave");
        perror(strerror(errno));
    }


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


    numeroNave=0;
    for(i=0;i<SO_PORTI;i++){
        if(portArrays[i].idPorto>pidPortoAlto)
            pidPortoAlto=portArrays[i].idPorto;
    }
    numeroNave=getpid()-pidPortoAlto;

    /*inizia il ciclo dei giorni */
    while(giorniSimulazioneNave<SO_DAYS){

        sigaction(SIGUSR1, &sa, 0);
        sigaction(SIGUSR2, &sa, 0);


        if(statoNave==0)
            report->senzaCarico+=1;
        else if(statoNave==1)
            report->conCarico+=1;
        if(statoNave==2)
            report->inPorto+=1;

        for(i=0;i<SO_MERCI;i++){
            report->merci[i]+=merciNave[i].quantita;
        }

        if(controllato==0){
            searchPort();
            controllato=1;
        }
        movimento();

        /* Set up the mask of signals to temporarily block. */
        /*sigemptyset (&my_mask);
        sigfillset(&my_mask);
        sigdelset(&my_mask, SIGUSR2);*/
        while (giornoAttuale == giorniSimulazioneNave)
            sigsuspend (NULL);
        giornoAttuale = giorniSimulazioneNave;

        giorniSimulazioneNave++;
        gestioneInvecchiamentoMerci();

    }

    exit(EXIT_SUCCESS);

}