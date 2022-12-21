
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include "utility.h"
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }
/* Processo nave */

float xNave = 0;
float yNave = 0;
float residuoCapacitaNave = SO_CAPACITY;
int xPortoMigliore=-1, yPortoMigliore=-1;
structMerce *merciNave; // puntatore all'array delle merci della nave

//TODO da finire di implementare, manca il controllo sul semaforo delle banchine MA PROBABILMENTE NON SARA NECESSARIO
void searchPort(portDefinition *portArrays, int merceRichiesta) {//array porti, array di merci della nave
    int i,k, valoreMerceMassimo = 0, banchinaLibera = 0; //coefficenteDistanza = distanza tra porti/merce massima, utilizzato per valutare la bontà della soluzione
    float coefficenteDistanza = 0, xAux = 0, yAux = 0;

    for (i = 0; i < SO_PORTI; i++) {
        for (k = 0; k < SO_MERCI; k++) {
            if (portArrays[i].merce[k].nomeMerce == merceRichiesta && portArrays[i].merce[k].offertaDomanda == 1) { //vedo se il porto propone la merce
                if (xNave > (float) portArrays[i].x)
                    xAux = xNave - (float) portArrays[i].x;
                else
                    xAux = (float) portArrays[i].x - xNave;
                if (yNave > (float) portArrays[i].y)
                    yAux = yNave- (float) portArrays[i].y;
                else
                    yAux = (float) portArrays[i].y - yNave;

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

void comunicazionePorto(){

}

void movimento(){

    if(xNave!=(float)xPortoMigliore || yNave!= (float)yPortoMigliore){
        if(xNave!=(float)xPortoMigliore){
            if(xNave<(float)xPortoMigliore){
                xNave+SO_SPEED;
                if(xNave>(float)xPortoMigliore)
                    xNave=(float)xPortoMigliore;//se dovesse andare troppo a destra di numero lo rispetto alla stessa x dato che idealmente la nave poi o gira o si ferma a quella x
            }else if(xNave>(float)xPortoMigliore){
                xNave-SO_SPEED;
                if(xNave<(float)xPortoMigliore)
                    xNave=(float)xPortoMigliore;//se dovesse andare troppo a sinistra di numero lo rispetto alla stessa x dato che idealmente la nave poi o gira o si ferma a quella x
            }
        }else if(xNave==(float)xPortoMigliore && yNave!= (float)yPortoMigliore){
            if(yNave<(float)yPortoMigliore){
                yNave+SO_SPEED;
                if(yNave>(float)yPortoMigliore)
                    yNave=(float)yPortoMigliore;//se dovesse andare troppo in su di numero lo rispetto alla stessa y dato che idealmente la nave poi o gira o si ferma a quella y
            }else if(yNave>(float)yPortoMigliore){
                yNave-SO_SPEED;
                if(yNave<(float)yPortoMigliore)
                    yNave=(float)yPortoMigliore;//se dovesse andare troppo in giu' di numero lo rispetto alla stessa y dato che idealmente la nave poi o gira o si ferma a quella y
            }
        }

    }else if(xNave==(float)xPortoMigliore && yNave== (float)yPortoMigliore){
        comunicazionePorto();
    }

}

int startNave(int argc, char *argv[]) {
    //printf("ciao \n");

    merciNave = malloc(sizeof(structMerce) * SO_MERCI);
    srand(time(NULL));

    merciNave->quantita = 0;
    merciNave->vitaMerce = 0;
    merciNave->nomeMerce = (rand() %  (int)SO_MERCI);
    merciNave->offertaDomanda = (rand() %  2);


    xNave=(rand() %  (int)SO_LATO);
    yNave=(rand() %  (int)SO_LATO);

    createIPCKeys();

    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;

    portArrayId = shmget(keyPortArray,size,0666);

    portArrays = shmat(portArrayId,NULL,0);
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray nel processo nave");
        perror(strerror(errno));
    }

    //searchPort(portArrays,(rand() %  (int)SO_MERCI));

    //movimento();

    return 0;

}
