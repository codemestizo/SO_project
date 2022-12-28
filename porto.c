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

int idSemBanchine,indicePorto;

//funzione che valorizza le informazioni per una singola struct dell'array di struct delle merci
//si assume che l'indice della cella dell'array corrisponda al nome della merce, sennò l'interpretazione non funziona

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
}

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
            if(commaCounter == 1)
                buf.mType = atoi(value);
            else if(commaCounter == 2 && portArrays[indicePorto].merce[indiceMerce].offertaDomanda == atoi(value)){
                strcpy(value," ");
            }else if(commaCounter == 2 && portArrays[indicePorto].merce[indiceMerce].offertaDomanda != atoi(value)){
                check = 0;
                strcpy(value," ");
            }else if(commaCounter == 3 && portArrays[indicePorto].merce[indiceMerce].nomeMerce != atoi(value)){
                strcpy(value," ");
            }
            else if(commaCounter == 4){
                portArrays[indicePorto].merce[indiceMerce].quantita = portArrays[indicePorto].merce[indiceMerce].quantita - atoi(value);
                strcpy(value," ");
            }

        }
    }

}

void comunicazioneNave(int numSemBanchina){

    char msg[10 * SO_MERCI];
    char workString[20];

    if (msgrcv(messageQueueId, &buf, sizeof(buf.mText), getpid(), IPC_NOWAIT) == -1) {
        perror("msgrcv");
        exit(1);
    }
    else{
        int indiceMerce = 0;
        for(int i = 0;i < strlen(buf.mText);i++){
            int commaCounter = 0;
            char c = buf.mText[i];
            if(c == ';')
                commaCounter++;
            if(commaCounter == 4){
                interpretaSezioneMessaggio(msg, indiceMerce);
                indiceMerce++;
                strcpy(msg," ");
            }
            else
                strcat(msg,&c);
        }
    }

    //for di creazione messaggio per il porto desiderato
    for(int i = 0;i < SO_MERCI;i++){
        sprintf(workString,"%d",portArrays[indicePorto].merce[i].offertaDomanda);
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%c",';');
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%f",portArrays[indicePorto].merce[i].quantita);
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%c",';');
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%d",portArrays[indicePorto].merce[i].vitaMerce);
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%c",';');
        strcat(msg,workString);
        strcpy(workString, "");
        strcpy(buf.mText,msg);
        strcpy(msg, "");
    }

    if((msgsnd(messageQueueId,&buf,sizeof(buf.mText),0))==-1){
        printf("Errore mentre faceva il messaggio");
        perror(strerror(errno));
    }else{
        printf("messaggio spedito");
        //settare semaforo a 2
    }

    if(releaseSem(idSemBanchine,numSemBanchina)==-1){
        printf("errore durante l'incremento del semaforo per scrivere sulla coda di messaggi in nave.c");
        perror(strerror(errno));
    }

}

void findScambi(){
    int numSem;
    for(int i=0;i<SO_BANCHINE;i++){
        numSem = semctl(idSemBanchine,i,GETVAL);
        if(numSem == 2)
            comunicazioneNave(i);
    }
}

// il semaforo è semPortArrayId id , devo poi chiamare la funzione reserve semaphore e gli passo portArrayid a reserve sem e secondo parametro 1
//devo fare release sem prima perchè mi torna la semop che prova a decrementare il semaforo di 1
//dopo aver fatto l'inserimento chiamo release semaphore che aumenta il valore di 1
//funzione che riempirà le struct dei porti

void setPorto(){

    semPortArrayId=  semget(keySemPortArray,1,IPC_CREAT | 0666);
    if(reserveSem( semPortArrayId, 0)==-1){ //richiede la memoria e la occupa SOLO LUI
        printf("errore durante il decremento del semaforo per inizializzare il porto");
        perror(strerror(errno));
    }
    //printf("sono in porto");


    int i=0;
    while(portArrays[i].idPorto!=0){
        i++;
    }
    if(portArrays[i].idPorto==0){
        indicePorto = i;
        portArrays[i].idPorto=getpid();
        portArrays[i].semIdBanchinePorto = semget(IPC_PRIVATE,SO_BANCHINE,IPC_CREAT | 0600);
        for(int j=0;i<SO_BANCHINE-1;j++){
            initSemAvailable(portArrays[i].semIdBanchinePorto,i);
        }

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
    if(releaseSem(semPortArrayId, 0)==-1){
        printf("errore durante l'incremento del semaforo dopo aver inizializzato il porto");
        perror(strerror(errno));
    }


}


void gestioneInvecchiamentoMerci(){ //funzione da richiamare ogni "giorno" di simulazione per checkare se la merce del porto è scaduta
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

    createIPCKeys();
    printf(" PID DI STO PROCESSO %d\n",getpid());
    printf("keyPortArray %d \n",keyPortArray);

    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;

    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        TEST_ERROR
    }
    portArrays = shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        TEST_ERROR
    }
    printf("test1");

    //stampaStatoMemoriaa();
    setPorto();
    /*for(int i = 0;i<SO_PORTI;i++){

        printf("X del porto %d: %d   \n",i,portArrays[i].x);
        printf("Y del porto %d: %d   \n",i,portArrays[i].y);
        printf("ID DEL PORTO :%d \n",portArrays[i].idPorto);
         for(int j=0;j<SO_MERCI;j++){
            printf("La merce numero %d e' richieta/offerta/non (%d)  in qualita' pari a :%f tonnellate con una vita (se venduta)  di %d giorni \n",portArrays[i].merce[j].nomeMerce,portArrays[i].merce[j].offertaDomanda,portArrays[i].merce[j].quantita,portArrays[i].merce[j].vitaMerce);
         }*/


    portArrays[0].merce[1].offertaDomanda=1;
    portArrays[0].merce[1].quantita=10;
    portArrays[0].merce[1].vitaMerce=40;

    if ((messageQueueId = msgget(keyMessageQueue, IPC_CREAT | 0644)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }
    printf("message queue: ready to receive messages.\n");


    while(SO_DAYS-giorniSimulazione>0){

        findScambi();

        printf("Giorno: %d.\n",giorniSimulazione);
        giorniSimulazione++;
        sleep(1);
    }

    return 0;
}






