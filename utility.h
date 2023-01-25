#ifndef UTILITY_H
#define UTILITY_H
/* Tutti i dati che saranno condivisi con gli altri processi*/
#define _GNU_SOURCE
#include <glob.h>

#define SO_NAVI 3//numero di navi che navigano
#define SO_PORTI 6 //numero di porti presenti
#define SO_MERCI 5 //tipi di merci diverse
#define SO_SIZE ( SO_FILL/SO_PORTI) //tonnellate di merci
#define SO_MIN_VITA 10 //giorni di vita  MIN della merce
#define SO_MAX_VITA 30 //giorni di vita  MAX della merce
#define SO_LATO 30 //lunghezza del lato della mappa (quadrata)
#define SO_SPEED 30 //KM AL GIORNO
#define SO_CAPACITY 3000 //Tonnellate che può caricare ogni nave
#define SO_BANCHINE 3 // Banchine che ha ogni porto
#define SO_FILL 10000 //Tonnellate totali di merci richieste e offerte da TUTTI i porti in totale
#define SO_LOADSPEED 500 //tonnellate al giorno per cui viene impegnata una banchina // velocità carico/scarico
#define SO_DAYS 10//giorni dopo quanto muore la simulazione
#define SO_MERCI_NAVE 2 //merci richieste dalla singola nave
#define SO_STORM_DURATION 6//ore per cui una nave sta ferma
#define SO_SWELL_DURATION 24//ore per cui un porto sta fermo
#define SO_MAELSTROM 64//ore ogni quanto affonda una nave


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
    int quantita;
    int nomeMerce;
}structMerce;

struct msgbuf {
    long int mType;
    char mText[256];
}; //messaggio con cui comunicheranno nave e porti



/* Struttura del record aggiunto da ogni processo *//*
struct record {
    pid_t  sender;
    char text[30];
};

/* Struttura del messaggio *//*
struct msgbuf {
    long int mtype;                   /* type of message */
 /*   unsigned int num_recs;        /* number of records */
   /* struct record rec_list[100];
};
*/
typedef struct {
    int x;
    int y;
    int idPorto;
    int semIdBanchinePorto;
    structMerce merce[SO_MERCI];
}portDefinition;
static int lungfifo;
static char fifo_name1[] = "reportFifo";
static  int fd;
static struct msgbuf *buf;
static portDefinition *portArrays;
static int portArrayId;
static int semPortArrayId;
static int semMessageQueueId;
static int messageQueueId;
static int semDaysId;
static int semPartiId;
static key_t keyPortArray;
static key_t keyMessageQueue;
static key_t keyGiorni;
static key_t keyStart;
static int keySemMessageQueue;
static int keySemPortArray;
static int *array;
static int shmid;
static int posizioneMerce=0;
static int leng=SO_PORTI*SO_MERCI;

static int sum; //usato per parificare il tot merci
//portDefinition * createPortArray();
void testo();

int controlloPosizione( int x, int y);

void generaMerce();
int initSemAvailable(int semId, int semNum);

int reserveSem(int semId, int semNum);

int releaseSem(int semId, int semNum);

void createPortArray();
#endif // UTILITY_H_
