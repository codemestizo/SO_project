#include <stdio.h>
#include <errno.h>
#include <sys/sem.h>
#include "utility.h"

#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }



void createPortArray(){
    int i;


    for(i=0;i<SO_PORTI;i++){
        portArrays[i].x = 0;
        portArrays[i].y = 0;
        portArrays[i].idPorto = 0;
    }
}

/*CONTROLLA SE LA NAVE E' SUL PORTO*/
int controlloPosizione( int x, int y){
    int portoAttuale;
    for(portoAttuale=0;portoAttuale<SO_PORTI;portoAttuale++){

        if( portArrays[portoAttuale].x == x && portArrays[portoAttuale].y == y){
            return portArrays[portoAttuale].idPorto;
        }
    }
    return -1;
}

/*codice preso dalle slide sull'utilizzo dei semafori,NON di nostra inventiva*/

/* Initialize semaphore to 1 (i.e., "available") */
int initSemAvailable(int semId, int semNum) {
    union semun arg;
    arg.val = 1;
    return semctl(semId, semNum, SETVAL, arg);
}

/* Reserve semaphore - decrement it by 1 */
int reserveSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}
/* Release semaphore - increment it by 1 */
int releaseSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}
