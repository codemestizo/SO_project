#ifndef UTILITY_H
#define UTILITY_H
/* Tutti i dati che saranno condivisi con gli altri processi*/
#include <glob.h>

#define SO_NAVI 50 /*numero di navi che navigano*/
#define SO_PORTI 100 /*numero di porti presenti*/
#define SO_MERCI 5 /*tipi di merci diverse*/
#define SO_SIZE ( SO_FILL/SO_PORTI) /*tonnellate di merci*/
#define SO_MIN_VITA 5 /*giorni di vita  MIN della merce*/
#define SO_MAX_VITA 15 /*giorni di vita  MAX della merce*/
#define SO_LATO 30 /*lunghezza del lato della mappa (quadrata)*/
#define SO_SPEED 30 /*KM AL GIORNO*/
#define SO_CAPACITY 300 /*Tonnellate che può caricare ogni nave*/
#define SO_BANCHINE 3 /* Banchine che ha ogni porto*/
#define SO_FILL 10000 /*Tonnellate totali di merci richieste e offerte da TUTTI i porti in totale*/
#define SO_LOADSPEED 500 /*tonnellate al giorno per cui viene impegnata una banchina // velocità carico/scarico*/
#define SO_DAYS 17/*giorni dopo quanto muore la simulazione*/
#define SO_MERCI_NAVE 2 /*merci richieste dalla singola nave*/
#define SO_STORM_DURATION 6/*ore per cui una nave sta ferma*/
#define SO_SWELL_DURATION 24/*ore per cui un porto sta fermo*/
#define SO_MAELSTROM 2000/*ore ogni quanto affonda una nave*/


union semun {
/* value for SETVAL */
    int val;
/* buffer for IPC_STAT, IPC_SET */
    struct semid_ds* buf;
/* array for GETALL, SETALL */
    unsigned short* array;
/* Linux specific part */
#if defined(__linux__)
    /* buffer for IPC_INFO */
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
}; /*messaggio con cui comunicheranno nave e porti */

typedef struct {
    int x;
    int y;
    int idPorto;
    int semIdBanchinePorto;
    structMerce merce[SO_MERCI];
}portDefinition;

typedef struct {

    int conCarico;
    int senzaCarico;
    int inPorto;
    int merci[SO_MERCI]; /*riferisce alle merci della nave*/
    int consegnataDaNave[SO_MERCI];
    int merciScaduteNave[SO_MERCI];
    int merciScadutePorto[SO_MERCI];
    int speditePorto[SO_PORTI];
    int ricevutePorto[SO_PORTI];
    int richieste[SO_PORTI];
    int offerte[SO_PORTI];
    int merciGenerate[SO_MERCI];
    int rallentate;
    int affondate;
    int rallentati;/*porti*/
    int banchine[SO_PORTI];
    int spediteOggi[SO_PORTI];
    int ricevuteOggi[SO_PORTI];
}reportStruct;





static char fifo_name1[] = "reportFifo";
/*static struct msgbuf *buf; */
static portDefinition *portArrays;
static reportStruct *report;
static int portArrayId;
static int reportId;
static int semPortArrayId;
static int semMessageQueueId;
static int messageQueueId;
static int semDaysId;
static int semPartiId;
static key_t keyPortArray;
static key_t keyReport;
static key_t keyMessageQueue;
static key_t keyGiorni;
static key_t keyStart;
static int keySemMessageQueue;
static int keySemPortArray;
static int *array;
static int sum; /*usato per parificare il tot merci*/

void testo();

int controlloPosizione( int x, int y);

void generaMerce();
int initSemAvailable(int semId, int semNum);

int reserveSem(int semId, int semNum);

int releaseSem(int semId, int semNum);

void createPortArray();
#endif /* UTILITY_H_ */