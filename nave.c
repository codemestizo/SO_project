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

float xNave = 0;
float yNave = 0;
float residuoCapacitaNave = SO_CAPACITY;
float xPortoMigliore=-1, yPortoMigliore=-1;
structMerce *merciNave; // puntatore all'array delle merci della nave
int pidPortoDestinazione;
//TODO da finire di implementare, manca il controllo sul semaforo delle banchine MA PROBABILMENTE NON SARA NECESSARIO

void searchPort( ) {//array porti, array di merci della nave
    int i,k, valoreMerceMassimo = 0, banchinaLibera = 0; //coefficenteDistanza = distanza tra porti/merce massima, utilizzato per valutare la bontà della soluzione
    float coefficente = 0, xAux = 0, yAux = 0;
    float attualeMigliore=0;
    float distanza=0;
    float vita=0;
    float occupato=0;
    float parificatore=0;//serve per il coefficiente (per veder ESCLUSIVAMENTE la merce scambiata quando ne avanza da porto/nave), per renderlo più efficiente
    for (i = 0; i < SO_PORTI; i++) {
        occupato=0;
        coefficente=0;
        distanza=(xNave+yNave)-(portArrays[i].x+portArrays[i].y);
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
        }


    }

    printf("Il porto migliore si trova a %f , %f",xPortoMigliore,yPortoMigliore);
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
            }else if(commaCounter == 2){
                merciNave[indiceMerce].quantita = merciNave[indiceMerce].quantita - atoi(value);
                if(merciNave[indiceMerce].quantita < 0)
                    merciNave[indiceMerce].quantita = 0;
                strcpy(value," ");
            }
            else{
                merciNave[indiceMerce].vitaMerce = atoi(value);
                strcpy(value," ");
            }

        }
    }

}

void comunicazionePorto(){

    //buf.offertaDomanda=merciNave->offertaDomanda;
    //buf.nomeMerce=merciNave->nomeMerce;
    //buf.quantita=merciNave->quantita;

    for(int i=0;i<SO_PORTI;i++){
        if(xNave == (float) portArrays[i].x && yNave == (float) portArrays[i].y)
            pidPortoDestinazione = portArrays[i].idPorto;
    }

    buf.mType = pidPortoDestinazione;
    char msg[10 * SO_MERCI];
    char workString[10];
    for(int i = 0;i < SO_MERCI;i++){
        sprintf(workString,"%d",merciNave[i].offertaDomanda);
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%c",';');
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%d",merciNave[i].nomeMerce);
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%c",';');
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%f",merciNave[i].quantita);
        strcat(msg,workString);
        strcpy(workString, "");
        strcpy(buf.mText,msg);
        strcat(msg,workString);
        strcpy(workString, "");
    }

    if((msgsnd(messageQueueId,&buf,sizeof(buf.mText),0))==-1){
        printf("Errore mentre faceva il messaggio");
        perror(strerror(errno));
    }else{
        printf("messaggio spedito");
        //settare semaforo a 2
    }
    strcpy(msg," ");

    //se semaforo a 3
    if (msgrcv(messageQueueId, &buf, 100, pidPortoDestinazione, IPC_NOWAIT) == -1) {
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
}


void movimento(){
    struct timespec tim, tim2;
    printf("Mi trovo a X nave: %f\n",xNave);
    printf("Mi trovo a Y nave: %f\n",yNave);
    if(xNave!=xPortoMigliore || yNave!= yPortoMigliore){
        if(xNave < xPortoMigliore && yNave < yPortoMigliore){
            tim.tv_sec = (int) (((xPortoMigliore - xNave) + (yPortoMigliore - yNave))/SO_SPEED);
            char str[10];
            sprintf(str,"%f",(((xPortoMigliore - xNave) + (yPortoMigliore - yNave))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
        }else if(xNave > xPortoMigliore && yNave < yPortoMigliore){
            tim.tv_sec = (int) (((xNave-xPortoMigliore) + (yPortoMigliore - yNave))/SO_SPEED);
            char str[10];
            sprintf(str,"%f",(((xNave - xPortoMigliore) + (yPortoMigliore - yNave))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
        }else if(xNave < xPortoMigliore && yNave > yPortoMigliore){
            tim.tv_sec = (int) (((xPortoMigliore-xNave) + (yNave - yPortoMigliore))/SO_SPEED);
            char str[10];
            sprintf(str,"%f",(((xPortoMigliore - xNave) + (yNave - yPortoMigliore))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
        }else if(xNave > xPortoMigliore && yNave > yPortoMigliore){
            tim.tv_sec = (int) (((xNave-xPortoMigliore) + (yNave - yPortoMigliore))/SO_SPEED);
            char str[10];
            sprintf(str,"%f",(((xNave-xPortoMigliore) + (yNave - yPortoMigliore))/SO_SPEED) - (int) tim.tv_sec);
            sprintf(str,"%s",&str[2]);
            strcat(str,"00L");
            tim.tv_nsec = atoi(str);
            nanosleep(&tim,&tim2);
        }
        xNave = xPortoMigliore;
        yNave = yPortoMigliore;
        movimento();
    }else if(xNave==xPortoMigliore && yNave== yPortoMigliore){
        comunicazionePorto();
    }

}





int startNave(int argc, char *argv[]) {
    //printf("Starto nave \n");



    srand(time(NULL));

    xNave=(rand() %  (int)SO_LATO);
    yNave=(rand() %  (int)SO_LATO);

    messageQueueId=msgget(keyMessageQueue, IPC_CREAT | 0666)  ; //ottengo la coda di messaggi

    printf("X nave: %f\n",xNave);
    printf("Y nave: %f\n",yNave);
    merciNave = malloc(sizeof(structMerce) * SO_MERCI);
    merciNave->quantita = 10;
    merciNave->vitaMerce = 0;
    merciNave->nomeMerce = 1;
    merciNave->offertaDomanda = 0;
    printf("Alla nave serve la merce numero %d \n",merciNave->nomeMerce);

    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;

    portArrayId = shmget(keyPortArray,size,0666);

    portArrays = shmat(portArrayId,NULL,0);
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray nel processo nave");
        perror(strerror(errno));
    }


    printf("ei sono qui nave \n");

    searchPort();
    movimento();



    return 0;

}

