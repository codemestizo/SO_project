//
// Created by daddy on 12/13/22.
//
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



/* Tutti i dati che saranno condivisi con gli altri processi*/

#define SO_NAVI 1 //numero di navi che navigano
#define SO_PORTI 4 //numero di porti presenti
#define SO_MERCI 2 //tipi di merci diverse
#define SO_SIZE 1 //tonnellate di merci
#define SO_MIN_VITA 10 //giorni di vita  MIN della merce
#define SO_MAX_VITA 30 //giorni di vita  MAX della merce
#define SO_LATO 10.0 //lunghezza del lato della mappa (quadrata)
#define SO_SPEED 1.0 //KM AL GIORNO
#define SO_CAPACITY 10 //Tonnellate che può caricare ogni nave
#define SO_BANCHINE 3 // Banchine che ha ogni porto
#define SO_FILL 200000 //Tonnellate totali di merci richieste e offerte da TUTTI i porti in totale
#define SO_LOADSPEED 200 //tonnellate al giorno per cui viene impegnata una banchina // velocità carico/scarico
#define SO_DAYS 10 //giorni dopo quanto muore il processo





typedef struct  {
    int offertaDomanda;
    char *vitaMerce;
    int quantita;
    char *nomeMerce;
}structMerce;


typedef struct {
    float x;
    float y;
    int idPorto;
    float quantitaMerce;
    char *nomeMerce;
    structMerce *merce;
}portDefinition;

typedef struct {
    size_t size;
    portDefinition *ports;
}Array;


static Array *portArray[SO_PORTI];





void createPortArray(){

    int shm_id = 0;

    shm_id = shmget(IPC_PRIVATE,SO_PORTI * sizeof(portDefinition),0666); // crea la shared memory con shmget

    &portArray->ports = shmat(shm_id,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo

    //portArray->ports = (portDefinition *)malloc(SO_PORTI * sizeof(portDefinition));



    portArray->size = SO_PORTI;

}



//CONTROLLA SE LA NAVE E' SUL PORTO
int controlloPosizione( float x, float y){ //in teoria è giusto TODO check se è giusto (a livello di come punta e logica)
    int portoAttuale=0; //contatore
    for(portoAttuale=0;portoAttuale<SO_PORTI;portoAttuale++){

        if( portArray[portoAttuale]->x == nave->x && portArray[portoAttuale]->y == nave->y){
            return portoAttuale; //ritorna il porto n su cui si trova la nave o dovrò tornare il PID di quel porto?
        }
    }

    return -1;// se -1 non è su nessun porto
}




int main(int argc, char** argv){
    //TODO testare che venga creata la sharedmemory e che sia correttamente istanziata(occhio, son poco sicuro che funzioni la structMerce)
    //ricordatevi che questo main è temporaneo, una volta sicuri che funziona il file utility va eliminato (il main)
}



