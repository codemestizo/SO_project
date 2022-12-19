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

#include "Utility.h"


 // il semaforo è semPortArrayId id , devo poi chiamare la funzione reserve semaphore e gli passo portArrayid a reserve sem e secondo parametro 1
 //devo fare release sem prima perchè mi torna la semop che prova a decrementare il semaforo di 1
 //dopo aver fatto l'inserimento chiamo release semaphore che aumenta il valore di 1
//funzione che riempirà le struct dei porti

void setPorto(portDefinition *portArrays){
    /*if(reserveSem( semPortArrayId, 1)==-1){ //richiede la memoria e la occupa SOLO LUI
        printf("errore durante il decremento del semaforo per inizializzare il porto");
        perror(strerror(errno));
    }*/
    int i=0;
    while(portArrays[i].idPorto!=0){
        i++;
    }
    if(portArrays[i].idPorto==0){
        portArrays[i].idPorto=getpid();
        portArrays[i].semIdBanchinePorto = semget(IPC_PRIVATE,SO_BANCHINE,0600);

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

int main(int argc, char *argv[]){
    printf("niga \n");
    portDefinition *portArrays = shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo

}







