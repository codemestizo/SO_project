
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include "Utility.h"
/* Processo nave */

int x = 0;
int y = 0;

int main(int argc, char** argv){
    int scadenzaMerce = 0;
    srand(time(NULL));
    x=(rand() %  (int)SO_LATO);
    y=(rand() %  (int)SO_LATO);

    Array *portArray = shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo
    if (portArray == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }

}
//da finire di implementare, per ora non fa niente di corretto
void searchPort(Array *portArray){
    int i,j, xAux=0, yAux=0, valoreMerceMassimo=0, banchinaLibera = 0; //da x a banchinaLibera sono variabili utilizzate per la ricerca del porto migliore

    for(i=0;i<SO_PORTI;i++){
        portArray[i].ports = malloc(sizeof(portDefinition));

        for(j=0;j<SO_MERCI;j++){
            portArray[i].ports[j].merce = malloc(sizeof(structMerce));
        }

    }

}


