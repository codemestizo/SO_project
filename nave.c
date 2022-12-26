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
void searchPort(int merceRichiesta) {//array porti, array di merci della nave
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

                if (coefficenteDistanza < portArrays[i].merce[k].quantita/((xAux + yAux)) &&
                        (SO_SPEED*(xAux + yAux)) < (float) portArrays[i].merce[k].vitaMerce) { //qua bisogna moltiplicare la distanza per la velocità delle navi
                    xPortoMigliore = portArrays[i].x;
                    yPortoMigliore = portArrays[i].y;
                    coefficenteDistanza =  portArrays[i].merce[k].quantita/((xAux + yAux));
                    pidPortoDestinazione=portArrays[i].idPorto;
                }


            }
        }

    }

    printf("Il porto migliore si trova a %f , %f",xPortoMigliore,yPortoMigliore);

}
void comunicazionePorto(){
    buf.idPorto=pidPortoDestinazione;
    buf.offertaDomanda=merciNave->offertaDomanda;
    buf.nomeMerce=merciNave->nomeMerce;
    buf.quantita=merciNave->quantita;


    if((msgsnd(messageQueueId,&buf,100,0))==-1){
        printf("Errore mentre faceva il messaggio");
        perror(strerror(errno));
    }else printf("messaggio spedito");

    if (msgctl(messageQueueId, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }


    if (msgrcv(messageQueueId, &buf, 100, 0, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    merciNave->nomeMerce=buf.nomeMerce;
    merciNave->quantita=buf.quantita;
    merciNave->offertaDomanda=1;



}

void movimento(){
    printf("Mi trovo a X nave: %f\n",xNave);
    printf("Mi trovo a Y nave: %f\n",yNave);
    if(xNave!=xPortoMigliore || yNave!= yPortoMigliore){
        if(xNave!=xPortoMigliore){
            if(xNave<xPortoMigliore){
                xNave+=SO_SPEED;
                if(xNave>xPortoMigliore)
                    xNave=xPortoMigliore;//se dovesse andare troppo a destra di numero lo rispetto alla stessa x dato che idealmente la nave poi o gira o si ferma a quella x
            }else if(xNave>xPortoMigliore){
                xNave-=SO_SPEED;
                if(xNave<xPortoMigliore)
                    xNave=xPortoMigliore;//se dovesse andare troppo a sinistra di numero lo rispetto alla stessa x dato che idealmente la nave poi o gira o si ferma a quella x
            }
        }else if(xNave==xPortoMigliore && yNave!= yPortoMigliore){
            if(yNave<yPortoMigliore){
                yNave+=SO_SPEED;
                if(yNave>yPortoMigliore)
                    yNave=yPortoMigliore;//se dovesse andare troppo in su di numero lo rispetto alla stessa y dato che idealmente la nave poi o gira o si ferma a quella y
            }else if(yNave>yPortoMigliore){
                yNave-=SO_SPEED;
                if(yNave<yPortoMigliore)
                    yNave=yPortoMigliore;//se dovesse andare troppo in giu' di numero lo rispetto alla stessa y dato che idealmente la nave poi o gira o si ferma a quella y
            }
        }
        sleep(1);
        movimento();
    }else if(xNave==xPortoMigliore && yNave== yPortoMigliore){
        comunicazionePorto();
    }

}





int startNave(int argc, char *argv[]) {
   printf("Starto nave \n");


    srand(time(NULL));

    xNave=(rand() %  (int)SO_LATO);
    yNave=(rand() %  (int)SO_LATO);

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



    searchPort(merciNave->nomeMerce);
    movimento();



    return 0;

}


