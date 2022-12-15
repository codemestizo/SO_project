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

#include "Utility.c"


 // il semaforo è semPortArrayId id , devo poi chiamare la funzione reserve semaphore e gli passo portArrayid a reserve sem e secondo parametro 1
 //devo fare release sem prima perchè mi torna la semop che prova a decrementare il semaforo di 1
 //dopo aver fatto l'inserimento chiamo release semaphore che aumenta il valore di 1
//funzione che riempirà le struct dei porti

void setPorto(){
    reserveSem( semPortArrayId, 1); //richiede la memoria e la occupa SOLO LUI
    int i=0;
    while(portArray[i].ports->idPorto!=0){
        i++;
    }
    if(portArray[i].ports->idPorto==0){
        portArray[i].ports->idPorto=getpid();

        if(i==0){ //set spawn porto
            portArray[i].ports->x=0;
            portArray[i].ports->y=0;
        }else if(i==1){
            portArray[i].ports->x=SO_LATO;
            portArray[i].ports->y=0;
        }else if(i==2){
            portArray[i].ports->x=SO_LATO;
            portArray[i].ports->y=SO_LATO;
        }else if(i==3){
            portArray[i].ports->x=0;
            portArray[i].ports->y=SO_LATO;
        }else {
            srand(time(NULL));
            portArray[i].ports->x=(rand() %  (int)SO_LATO);
            portArray[i].ports->y=(rand() %  (int)SO_LATO);
            for(int j=0;j<i-1;j++){ //controllo che non spawni sulla posizione di un altro porto
                if(portArray[i].ports->x== portArray[j].ports->x && portArray[i].ports->y==portArray[j].ports->y){
                    j=-1;
                    portArray[i].ports->x=(rand() %  (int)SO_LATO);
                    portArray[i].ports->y=(rand() %  (int)SO_LATO);
                }

            }
        }
}
    for(int k=0;k<SO_MERCI;k++){
        srand(time(NULL));
        portArray[i].ports[k].merce->nomeMerce = k;
        portArray[i].ports[k].merce->offertaDomanda = (rand() %  2);//0 = domanda, 1 = offerta, 2 = da assegnare
        if(portArray[i].ports[k].merce->offertaDomanda ==1)
            portArray[i].ports[k].merce->vitaMerce = (SO_MIN_VITA + (rand() %  (SO_MAX_VITA-SO_MIN_VITA))); //giorni di vita
        portArray[i].ports[k].merce->quantita = rand() %  (SO_FILL/40); //TODO CAPIRE COME FAR SI CHE IL TOTALE DI TUTTI I PORTI FACCIA SOFILL


    }


    releaseSem(semPortArrayId, 1);
}



