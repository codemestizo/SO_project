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
/* Processo nave */
int comunicato=0;
int controllato=0;
int xNave = 0;
int yNave = 0;
int giorniSimulazioneNave = 0;
int numeroNave;
int com=0;
float residuoCapacitaNave = SO_CAPACITY;
int xPortoMigliore=-1, yPortoMigliore=-1;
structMerce *merciNave; // puntatore all'array delle merci della nave
int pidPortoDestinazione, idSemBanchine;
//TODO da finire di implementare, manca il controllo sul semaforo delle banchine MA PROBABILMENTE NON SARA NECESSARIO

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

void searchPort( ) {//array porti, array di merci della nave
    int i,k, valoreMerceMassimo = 0, banchinaLibera = 0; //coefficenteDistanza = distanza tra porti/merce massima, utilizzato per valutare la bontà della soluzione
    float coefficente = 0, xAux = 0, yAux = 0;
    float attualeMigliore=0;
    int distanza=0;
    float vita=0;
    float occupato=0;
    float parificatore=0;//serve per il coefficiente (per veder ESCLUSIVAMENTE la merce scambiata quando ne avanza da porto/nave), per renderlo più efficiente
    for (i = 0; i < SO_PORTI; i++) {
        occupato=0;
        coefficente=0;
        distanza=(xNave+yNave)-(portArrays[i].x+portArrays[i].y);
        printf("\nPorto %d la distanza vale %d \n",i,distanza);
        printf("\nPorto %d la x vale %d \n",i,portArrays[i].x);
        printf("\nPorto %d la y vale %d \n ",i,portArrays[i].y);
        if(distanza<0)
            distanza=distanza*(-1);
        for (k = 0; k < SO_MERCI; k++) {//0 = domanda, 1 = offerta, 2 = da assegnare
            if (merciNave[k].offertaDomanda==1 && portArrays[i].merce[k].offertaDomanda==0) { //vedo se il porto vuole la merce
                vita= merciNave[k].vitaMerce - (distanza/ SO_SPEED);

                if(merciNave[k].quantita<portArrays[i].merce[k].quantita){ //controllo che la merce sulla nave da vendere sia + o - di quella richiesta
                    parificatore=0;                                        // ovviamente se la nave ha più della merce richiesta, il coefficiente sarà minore in quanto sarà spostata meno merce
                }else if(merciNave[k].quantita>portArrays[i].merce[k].quantita)
                    parificatore=merciNave[k].quantita-portArrays[i].merce[k].quantita;

                if(vita>0) {
                    coefficente += ((merciNave[k].quantita-parificatore) / distanza);
                    occupato-=merciNave[k].quantita;
                }

            }else  if (merciNave[k].offertaDomanda==0 && portArrays[i].merce[k].offertaDomanda==1) {//vedo se il porto propone la merce
                vita= portArrays[i].merce[k].vitaMerce - (distanza/ SO_SPEED);

                if(merciNave[k].quantita<portArrays[i].merce[k].quantita){ //controllo che la merce nel porto da vendere sia + o - di quella richiesta
                    parificatore=0;                                        // ovviamente se il porto avesse più della merce richiesta, il coefficiente sarà minore in quanto sarà spostata meno merce
                }else if(merciNave[k].quantita>portArrays[i].merce[k].quantita)
                    parificatore=merciNave[k].quantita-portArrays[i].merce[k].quantita;



                if(vita>0 && occupato+portArrays[i].merce[k].quantita<SO_CAPACITY) {
                    coefficente += ((merciNave[k].quantita -parificatore)/ distanza);
                    occupato+=merciNave[k].quantita; //quantità che prenderebbe la nave
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

    printf("Il porto migliore si trova a %d , %d",xPortoMigliore,yPortoMigliore);
}




//funzione che valorizza le informazioni per una singola struct dell'array di struct delle merci
//si assume che l'indice della cella dell'array corrisponda al nome della merce, sennò l'interpretazione non funziona
void interpretaSezioneMessaggio(const char sezioneMessaggio[], int indiceMerce){
    int check = 1;

    for(int i = 0;i < strlen(sezioneMessaggio) && check != 0;i++){
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
    int numSem;
    for(int i=0;i<SO_BANCHINE;i++){
        numSem = semctl(idSemBanchine,i,GETVAL);
        if(numSem == 1)
            return i;
    }
    return -1;
}

void comunicazionePorto() {

    struct msgbuf buf;
    struct msgbuf buf1;
    //buf.offertaDomanda=merciNave->offertaDomanda;
    //buf.nomeMerce=merciNave->nomeMerce;
    //buf.quantita=merciNave->quantita;
    printf("\nentro in comunicazione");
    for (int i = 0; i < SO_PORTI; i++) {
        if (xNave == portArrays[i].x && yNave == portArrays[i].y)
            pidPortoDestinazione = portArrays[i].idPorto;
    }

    buf.mType = pidPortoDestinazione;
    char messaggio[30 * SO_MERCI];

    char workString[40];
    int banchinaRitorno;
    //for di creazione messaggio per il porto desiderato
    strcpy(workString, "");
    strcpy(messaggio, "");
    for (int i = 0; i < SO_MERCI; i++) {

        sprintf(workString, "%d|%d|%d|%d|", getpid(), merciNave[i].nomeMerce, merciNave[i].offertaDomanda,
                merciNave[i].quantita);
        strcat(messaggio, workString);
        strcpy(workString, "");

    }
    strcpy(buf.mText, messaggio);

    int numSemBanchina = findNumSem();
    printf("Il numero del numero semaforo panca è:%d", numSemBanchina);
    if (numSemBanchina == -1)
        exit(EXIT_FAILURE);

    if ((msgsnd(messageQueueId, &buf, sizeof(buf.mText), 0)) == -1) {
        printf("Errore mentre faceva il messaggio");
        TEST_ERROR;
    } else {
        printf("nave.c, messaggio spedito, pidPortoDestinazione %d\n", pidPortoDestinazione);
        printf("\nGLI MANDO IL MESSAGGIO %s", buf.mText);
        printf(" coda messaggi %d che spedisce", messageQueueId);
        banchinaRitorno = idSemBanchine;



        if (releaseSem(idSemBanchine, numSemBanchina) == -1) {
            printf("errore durante l'incremento del semaforo per scrivere sulla coda di messaggi in nave.c");
            TEST_ERROR;
        }
        printf("\nnave, incremento il semaforo banchina per porto, idSemBanchine: %d\n numSemBanchina: %d\n",
               idSemBanchine, numSemBanchina);
    }
printf("\nPASCIU %d pasciuuuuuuuu %d",giorniSimulazioneNave,semctl(semDaysId,SO_PORTI,GETVAL));
//if(giorniSimulazioneNave<=SO_DAYS){
    while(semctl(semDaysId,SO_PORTI,GETVAL) < giorniSimulazioneNave+1){
        if (releaseSem(semDaysId, SO_PORTI) == -1) {
            printf("errore durante l'incremento del semaforo per incrementare i giorni in nave ");
            TEST_ERROR;
        }com=1;
    }
    printf("\nPASCIaaaaaaa %d",semctl(semDaysId,SO_PORTI,GETVAL));

    //giorniSimulazioneNave++;
    //TODO dopo aver fixato in porto.c la comunicazione con la nave, testare la ricezione/*
    while(semctl(idSemBanchine,numSemBanchina,GETVAL) != 3){

    }
    printf("Io vedo che il sem va a 3");
    if (msgrcv(messageQueueId, &buf1, sizeof(buf1.mText), getpid(), IPC_NOWAIT) == -1) {
        TEST_ERROR;
        exit(EXIT_FAILURE);
    }
    else{
        printf(" ricevo il messaggio dio pera %s",buf1.mText);
    }
    numSemBanchina = 0;
    idSemBanchine = 0;
    initSemAvailable(idSemBanchine,numSemBanchina);



    //qua vado a decifrare il messaggio e settare nave
    char delim[] = "|";
    int scadenza = 0;
    int quantitaAttuale = 0;
    int ron = 2;//richiesta offerta non
    int nomeMerceChiesta = 0;
    char *ptr = strtok(buf1.mText, delim);
    char tmp[20];
    int pidPort = 0;
    int scadenzaAttuale = 0;
    int sep = 0;
    sep++;
    while (ptr != NULL) {
        if (sep == 1) {
            printf("\nIL PID DEL PORTO A CUI SIAMO STATI E': '%s' ", ptr);
            strcpy(tmp, ptr);
            pidPort = atoi(tmp);
        } else if (sep == 2) {
            strcpy(tmp, ptr);
            nomeMerceChiesta = atoi(tmp);
            printf("Merce numero: '%d' : ", nomeMerceChiesta);
        } else if (sep == 3) {
            strcpy(tmp, ptr);
            ron = atoi(tmp);
            if (ron == 0)
                printf(" e' richieta ");
            else if (ron == 1)
                printf(" e' in vendita ");
            else
                printf(" non è di interesse della nave");
        } else if (sep == 4) {
            strcpy(tmp, ptr);
            quantitaAttuale = atoi(tmp);
            printf(" in '%d' tonnellate ", quantitaAttuale);
        } else if (sep == 5) {
            strcpy(tmp, ptr);
            scadenzaAttuale = atoi(tmp);
            if (ron == 2)
                scadenzaAttuale = 0;
            printf(" scade tra '%d' giorni ", quantitaAttuale);
            merciNave[nomeMerceChiesta].offertaDomanda = ron;
            merciNave[nomeMerceChiesta].quantita = quantitaAttuale;
            merciNave[nomeMerceChiesta].vitaMerce = scadenzaAttuale;
            sep = 0;
        }
        ptr = strtok(NULL, delim);
        sep++;
    }
    comunicato=0;
    controllato=0; //se ne va a cercare un altro porto
 //}
}

void movimento(){

    struct timespec tim, tim2;
    printf("\nMi trovo a X nave: %d\n",xNave);
    printf("Mi trovo a Y nave: %d\n",yNave);
    printf("X port: %d\n",xPortoMigliore);
    printf("Y port: %d\n",yPortoMigliore);
    int giorniViaggio=((xNave+yNave)-(xPortoMigliore+yPortoMigliore))/SO_SPEED;
    if(xNave!=xPortoMigliore || yNave!= yPortoMigliore){
        printf("entroo");
        if(xNave < xPortoMigliore && yNave < yPortoMigliore){
            printf("<<<<<<<<<<");
            tim.tv_sec = (int) (((xPortoMigliore - xNave) + (yPortoMigliore - yNave))/SO_SPEED);
            char str[10];
            sprintf(str,"%d",(((xPortoMigliore - xNave) + (yPortoMigliore - yNave))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
            printf("muoio 1");
        }else if(xNave > xPortoMigliore && yNave < yPortoMigliore){
            printf(">>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<");
            tim.tv_sec = (int) (((xNave-xPortoMigliore) + (yPortoMigliore - yNave))/SO_SPEED);
            char str[10];
            sprintf(str,"%d",(((xNave - xPortoMigliore) + (yPortoMigliore - yNave))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
            printf("muoio 2");
        }else if(xNave < xPortoMigliore && yNave > yPortoMigliore){
            printf("<<<<<<<<<<>>>>>>>>>>>");
            tim.tv_sec = (int) (((xPortoMigliore-xNave) + (yNave - yPortoMigliore))/SO_SPEED);
            char str[10];
            sprintf(str,"%d",(((xPortoMigliore - xNave) + (yNave - yPortoMigliore))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
            printf("muoio 3");
        }else if(xNave > xPortoMigliore && yNave > yPortoMigliore){
            printf(">>>>>>>>>");
            tim.tv_sec = (int) (((xNave-xPortoMigliore) + (yNave - yPortoMigliore))/SO_SPEED);
            char str[10];
            sprintf(str,"%d",(((xNave-xPortoMigliore) + (yNave - yPortoMigliore))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
            printf("muoio 4");
        }

        xNave = xPortoMigliore;
        yNave = yPortoMigliore;

        //comunicazionePorto();
    }else if(xNave==xPortoMigliore && yNave== yPortoMigliore && comunicato<1){
       /* for(int i=1;i<giorniViaggio-1;i++) {
            while (semctl(semDaysId, SO_PORTI, GETVAL) != giorniSimulazioneNave + 1) {
                if (releaseSem(semDaysId, SO_PORTI ) == -1) {
                    printf("errore durante l'incremento del semaforo per incrementare i giorni in nave ");
                    TEST_ERROR;
                }
            }

            while (semctl(semPartiId, 0, GETVAL) < giorniSimulazioneNave + 1) {}
            giorniSimulazioneNave++;
        }*/

        printf("\ngiorni simulation prima di com port %d",giorniSimulazioneNave);
        printf("\ngiorni simulation prima di com port SECONDO SEMAFORO%d",  semctl(semDaysId, SO_PORTI, GETVAL));
        comunicato+=1;
        comunicazionePorto();

    }

}



void generaNave(){
    srand(getpid());
    xNave=(rand() %  SO_LATO);
    yNave=(rand() %  SO_LATO);
    int utile=0;
    /*for(int i=0;i<SO_MERCI;i++){

        merciNave[i].vitaMerce = 0;
        merciNave[i].nomeMerce = i;
        merciNave[i].offertaDomanda = (rand() %  3);//0 = domanda, 1 = offerta, 2 = da assegnare
        merciNave[i].quantita = (float)(rand() % ((int)SO_CAPACITY/SO_MERCI));//(float)(rand() % ((int)SO_CAPACITY/SO_MERCI)); //((int)SO_CAPACITY/SO_MERCI)
        if(merciNave[i].offertaDomanda !=0)
        {
            merciNave[i].quantita = 0.0;
            merciNave[i].offertaDomanda =2;
        }
        if(merciNave[i].offertaDomanda !=2)
            utile=1;
    }*/

    while(utile==0){ //ciò mi permette  che la nave spawni con almeno una richiesta, e che non sia inutile il suo spawn (in caso sarebbero risorse cpu sprecate)
        for(int i=0;i<SO_MERCI;i++){
            merciNave[i].vitaMerce = 0;
            merciNave[i].nomeMerce = i;
            merciNave[i].offertaDomanda = (rand() %  3);//0 = domanda, 1 = offerta, 2 = da assegnare
            merciNave[i].quantita = (float)(rand() % ((int)SO_CAPACITY/SO_MERCI));//(float)(rand() % ((int)SO_CAPACITY/SO_MERCI)); //((int)SO_CAPACITY/SO_MERCI)
            if(merciNave[i].offertaDomanda !=0)
            {
                merciNave[i].quantita = 0.0;
                merciNave[i].offertaDomanda =2;
            }
            if(merciNave[i].offertaDomanda !=2)
                utile=1;
        }
    }

}

void startNave(int argc, char *argv[]) {



    createIPCKeys();
    messageQueueId=msgget(keyMessageQueue, 0) ; //ottengo la coda di messaggi
    merciNave = malloc(sizeof(structMerce) * SO_MERCI);
    generaNave();
    semDaysId=  semget(keyGiorni,SO_PORTI+SO_NAVI,IPC_CREAT | 0666); //creo semafori gestione giorni
    if(semDaysId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }

    semPartiId=  semget(keyStart,1,IPC_CREAT | 0666); //creo semaforo per far partire i giorni
    if(semPartiId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }


    printf("X nave: %d\n",xNave);
    printf("Y nave: %d\n",yNave);
    printf("PID DELLA NAVE %d",getpid());
    char out[100];
    for(int i=0;i<SO_MERCI;i++){//, )
        sprintf(out, "\nLa merce %d e' richiesta/venduta/non da contare (0,1,2) --> %d  in  %d tonnellate\n", merciNave[i].nomeMerce, merciNave[i].offertaDomanda,(int)merciNave[i].quantita);
        puts(out);
    }

    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;

    portArrayId = shmget(keyPortArray,size,0);

    portArrays = shmat(portArrayId,NULL,0);
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray nel processo nave");
        perror(strerror(errno));
    }
    numeroNave=0;

    printf("\nsemday %d  semnum %d",semDaysId,numeroNave);
    while(SO_DAYS-giorniSimulazioneNave>0){
        printf("\nGiorno di nave: %d.\n",giorniSimulazioneNave);

        if(controllato==0){
            searchPort();
            controllato=1; //poi la devo mettere per tutta la nave e non solo nel main
        }
        movimento();
        printf("MI BLOCCO DOPO MOVIMENTO");
        printf("\nnumero nave %d",semDaysId);
        printf("\nnumero sincronizzatore %d",semPartiId);
        // printf("il pid della nave è %d e quello dell'ultimo porto %d",getpid(),portArrays[SO_PORTI-1].idPorto);

    if(com!=1){
        while(semctl(semDaysId,SO_PORTI,GETVAL) < giorniSimulazioneNave+1){
            if (releaseSem(semDaysId, SO_PORTI) == -1) {
                printf("errore durante l'incremento del semaforo per incrementare i giorni in nave ");
                TEST_ERROR;
            }
            printf("\ndioboia %d",semctl(semDaysId,SO_PORTI,GETVAL));
            printf("  giorni sim %d",giorniSimulazioneNave);
        }
    }
        com=0;
        printf("sincronizzatore giorni %d a giorno: %d",giorniSimulazioneNave,semctl(semPartiId,0,GETVAL));
      //  if(giorniSimulazioneNave!=SO_DAYS-1) {
      //if(com!=1)
            while (semctl(semPartiId, 0, GETVAL) != giorniSimulazioneNave + 1) {}

            giorniSimulazioneNave++;
        printf("\nEL GIORNO DI NAVE CONTATOR %d",giorniSimulazioneNave);
       // }

    }
    exit(EXIT_SUCCESS);

}
