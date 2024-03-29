#ifndef UTILITY_H
#define UTILITY_H
/* Tutti i dati che saranno condivisi con gli altri processi*/
#include <glob.h>

/*SI INVITA L'UTENTE A NON INSERIRE PARAMETRI CHE POSSANO COMPORTARE SOVRACCARICO O COMPORTAMENTI BIZZARRI. SI SUGGERISCE DI UTILIZZARE VALORI MODESTI (RELATIVI ALLA POTENZA DELLA MACCHINA SU CUI SI ESEGUE LA SIMULAZIONE)*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }
#define SO_PORTI 10 /*numero di porti presenti*/
#define SO_MERCI 5 /*tipi di merci diverse*/


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
    int merceSpedita;
    int merceRicevuta;
}structMerce;

struct messagebuf {
    long int mType;
    char mText[1024];
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
    int merci[SO_MERCI]; /*riferisce alle merci della nave*//*SO_MERCI*/
    int consegnataDaNave[SO_MERCI];/*SO_MERCI*/
    int merciScaduteNave[SO_MERCI];/*SO_MERCI*/
    int merciScadutePorto[SO_MERCI];/*SO_MERCI*/
    int speditePorto[SO_PORTI];/*SO_PORTI*/
    int ricevutePorto[SO_PORTI];/*SO_PORTI*/
    int migliorRichiesta;
    int offerte[SO_PORTI];
    int merciGenerate[SO_MERCI];/*SO_MERCI*/
    int rallentate;
    int affondate;
    int rallentati;/*porti*/
    int banchine[SO_PORTI];/*SO_PORTI*/
    int spediteOggi[SO_PORTI];/*SO_PORTI*/
    int ricevuteOggi[SO_PORTI];/*SO_PORTI*/
}reportStruct;

/*static struct msgbuf *buf; */
static portDefinition *portArrays;
static reportStruct *report;
static int portArrayId;
static int reportId;
static int semPortArrayId;
static int semMessageQueueId;
static int messageQueueId;
static key_t keyPortArray;
static key_t keyReport;
static key_t keyMessageQueue;

static int keySemMessageQueue;
static int keySemPortArray;
static int *array;
static int sum; /*usato per parificare il tot merci*/

void testo();

int controlloPosizione( int x, int y);

void generaMerce();
int initSemAvailable(int semId, int semNum);

int initSemAvailableTo0(int semId, int semNum);

int reserveSem(int semId, int semNum);

int releaseSem(int semId, int semNum, int nIncrement);

int waitSem(int semId, int semNum);

void clean();

#endif /* UTILITY_H_ */
