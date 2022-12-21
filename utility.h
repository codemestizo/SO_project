#ifndef UTILITY_H
#define UTILITY_H
/* Tutti i dati che saranno condivisi con gli altri processi*/

#include <glob.h>

#define SO_NAVI 1 //numero di navi che navigano
#define SO_PORTI 4 //numero di porti presenti
#define SO_MERCI 3 //tipi di merci diverse
#define SO_SIZE 1 //tonnellate di merci
#define SO_MIN_VITA 10 //giorni di vita  MIN della merce
#define SO_MAX_VITA 30 //giorni di vita  MAX della merce
#define SO_LATO 10.0 //lunghezza del lato della mappa (quadrata)
#define SO_SPEED 1.0 //KM AL GIORNO
#define SO_CAPACITY 10 //Tonnellate che può caricare ogni nave
#define SO_BANCHINE 3 // Banchine che ha ogni porto
#define SO_FILL 200000 //Tonnellate totali di merci richieste e offerte da TUTTI i porti in totale
#define SO_LOADSPEED 200 //tonnellate al giorno per cui viene impegnata una banchina // velocità carico/scarico
#define SO_DAYS 10 //giorni dopo quanto muore la simulazione
#define SO_MERCI_NAVE 1 //merci richieste dalla singola nave


union semun {
// value for SETVAL
    int val;
// buffer for IPC_STAT, IPC_SET
    struct semid_ds* buf;
// array for GETALL, SETALL
    unsigned short* array;
// Linux specific part
#if defined(__linux__)
// buffer for IPC_INFO
    struct seminfo* _buf;
#endif
};

typedef struct  {
    int offertaDomanda;
    int vitaMerce;
    float quantita;
    int nomeMerce;
}structMerce;

typedef struct  {
    int offertaDomanda;
    int id;
    float quantita;
    int nomeMerce;
}messaggio; //messaggio con cui comunicheranno nave e porti


typedef struct {
    int x;
    int y;
    int idPorto;
    int semIdBanchinePorto;
    structMerce merce[SO_MERCI];
}portDefinition;

static portDefinition *portArrays;
static int portArrayId;
static int semPortArrayId;
static int semMessageQueueId;
static int messageQueueId;
static int keyPortArray;
static int keyMessageQueue;
static int keySemMessageQueue;
static int keySemPortArray;


void createPortArray(portDefinition *portArrays);

void createIPCKeys();

int controlloPosizione( int x, int y, portDefinition *portArrays);

void generaMerce();

int initSemAvailable(int semId, int semNum);

int reserveSem(int semId, int semNum);

int releaseSem(int semId, int semNum);

#endif // UTILITY_H_