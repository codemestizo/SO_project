
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

float x = 0;
float y = 0;
float residuoCapacitaNave = SO_CAPACITY;
int xPortoMigliore=-1, yPortoMigliore=-1;
structMerce *merciNave; // puntatore all'array delle merci della nave

/*int main(int argc, char** argv){
    int scadenzaMerce = 0;
    srand(time(NULL));
    x=(rand() %  (int)SO_LATO);
    y=(rand() %  (int)SO_LATO);

    portArrayId = shmget(IPC_PRIVATE,SO_PORTI * sizeof(portDefinition),0666);// crea la shared memory con shmget
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        perror(strerror(errno));
    }
    portDefinition *portArrays = shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }

    merciNave = malloc(sizeof(structMerce) * SO_MERCI);
}*/
//TODO da finire di implementare, manca il controllo sul semaforo delle banchine MA PROBABILMENTE NON SARA NECESSARIO
void searchPort(portDefinition *portArrays, int merceRichiesta) {//array porti, array di merci della nave
    int richiesta = merceRichiesta;
    int i,k, valoreMerceMassimo = 0, banchinaLibera = 0; //coefficenteDistanza = distanza tra porti/merce massima, utilizzato per valutare la bontà della soluzione
    float coefficenteDistanza = 0, xAux = 0, yAux = 0;

    for (i = 0; i < SO_PORTI; i++) {
        for (k = 0; k < SO_MERCI; k++) {
            if (portArrays[i].merce[k].nomeMerce = richiesta && portArrays[i].merce[k].offertaDomanda == 1) { //vedo se il porto propone la merce
                if (x > (float) portArrays[i].x)
                    xAux = x - (float) portArrays[i].x;
                else
                    xAux = (float) portArrays[i].x - x;
                if (y > (float) portArrays[i].y)
                    yAux = y - (float) portArrays[i].y;
                else
                    yAux = (float) portArrays[i].y - y;

                //for (j = 0; j < SO_MERCI; j++) {
                if (coefficenteDistanza < portArrays[i].merce[k].quantita/((xAux + yAux)) &&
                        (SO_SPEED*(xAux + yAux)) < (float) portArrays[i].merce[k].vitaMerce) { //qua bisogna moltiplicare la distanza per la velocità delle navi
                    xPortoMigliore = portArrays[i].x;
                    yPortoMigliore = portArrays[i].y;
                    coefficenteDistanza =  portArrays[i].merce[k].quantita/((xAux + yAux));

                }
                //}

            }
        }

    }

}


