#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "utility.h"
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }



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

int initSemAvailableTo0(int semId, int semNum) {
    union semun arg;
    arg.val = 0;
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

/* waitTo0 */
int waitSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = 0;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}

void clean(){ /*dealloca dalla memoria*/

    int i,semVal;

    if(portArrays!=NULL){
        void *mem = shmat(portArrayId, 0, 0);
        void *memo = shmat(reportId, 0, 0);

        if (mem == (void *) -1){
            if(errno==EINVAL)
                printf("MEMORIA NON TROVATA");
            else
            TEST_ERROR
        }


        if (memo == (void *) -1){
            if(errno==EINVAL)
                printf("MEMORIA NON TROVATA");
            else
            TEST_ERROR
        }


        for(i=0;i<SO_PORTI;i++){
            if(  semctl(portArrays[i].semIdBanchinePorto,SO_BANCHINE,IPC_RMID)==-1){
                if (errno == EINVAL){

                    break;
                }

                else
                TEST_ERROR
            }
        }

/* 'remove' shared memory segment */
        if(shmctl(portArrayId, IPC_RMID, NULL) ==-1){
            if (errno == EINVAL)
                printf("portarray non trovato");
            else
            TEST_ERROR
        }

        if(shmctl(reportId, IPC_RMID, NULL) ==-1){
            if (errno == EINVAL)
                printf("semaforo non trovato");
            else
            TEST_ERROR
        }


    }
    /* shmctl(reportId, IPC_RMID, NULL);*/
    if (msgctl(messageQueueId, IPC_RMID, NULL)== -1) { /*cancella coda di messaggi*/
        if (errno == EINVAL)
            printf("CODA MESSAGGI non trovato");
        else
        TEST_ERROR
    }




    if(semctl(semPortArrayId,SO_PORTI,IPC_RMID)==-1) {
        if (errno == EINVAL)
            printf("semaforo non trovato");
        else
        TEST_ERROR
    }
    /* printf("\n ora pulisco i semafori dei processi");*/
    for(i=0;i<=SO_NAVI+SO_PORTI-1;i++){
        semVal=semctl(semDaysId,i,GETVAL);
        if(semVal!=-1){
            while(semctl(semDaysId,i,GETVAL)!=0)
                reserveSem(semDaysId, i);
        }else{
            break;
        }

    }
    if(semctl(semDaysId,SO_PORTI+SO_NAVI,IPC_RMID)==-1) {
        if (errno == EINVAL)
            printf("semaforo non trovato");
        else
        TEST_ERROR
    }

    if(semctl(semMessageQueueId,SO_PORTI,IPC_RMID)==-1) {
        if (errno == EINVAL)
            printf("semaforo non trovato");
        else
        TEST_ERROR
    }


}
