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
#define SO_LOADSPEED 200 //tonnellate al giorno
#define SO_DAYS 10 //giorni dopo quanto muore il processo

typedef struct  {
    int offertaDomanda;
    char *vitaMerce;
    char *nomeMerce;
}structMerce;


typedef struct {
    int x;
    int y;
    float lato;
    int idPorto;
    float quantitaMerce;
    int offertaDomanda;
    char *nomeMerce;
    structMerce *merce;
}portDefinition;

typedef struct {
    size_t used;
    size_t size;
    portDefinition *ports;
}Array;

void createPortArray(Array *portArray){

    int shm_id = 0;

    shm_id = shmget(IPC_PRIVATE,SO_PORTI * sizeof(portDefinition),0666);

    portArray->ports = shmat(shm_id,NULL,0);

    //portArray->ports = (portDefinition *)malloc(SO_PORTI * sizeof(portDefinition));

    portArray->used = 0;
    portArray->size = SO_PORTI;


}

int main(int argc, char** argv){
    //TODO testare che venga creata la sharedmemory e che sia correttamente istanziata(occhio, son poco sicuro che funzioni la structMerce)
    //ricordatevi che questo main è temporaneo, una volta sicuri che funziona il file utility va eliminato
}



