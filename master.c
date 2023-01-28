#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <signal.h>
#include "utility.h"

struct stat st;
int giorniSimulazione = 0,processiMorti=0;
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }


void createIPCKeys(){
    keyPortArray = ftok("master.c", 'u');
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyPortArray");
    }

    keySemPortArray = ftok("master.c", 'm');
    if(keySemPortArray == -1){
        TEST_ERROR
        perror("errore keySemPortArray");
    }
    keyMessageQueue = ftok("master.c", 'p');
    if(keyMessageQueue == -1){
        TEST_ERROR
        perror("errore keyMessageQueue");
    }
    keySemMessageQueue = ftok("master.c", 'n');
    if(messageQueueId == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }

    keyGiorni = ftok("master.c", 'o');
    if(semDaysId == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }

    keyStart = ftok("master.c", 'g');
    if(semPartiId == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }

    keyReport = ftok("master.c", 'r');
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyPortArray");
    }
}

void fillAndCreate_resource(){

    int i;
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        perror(strerror(errno));
    }

    portArrays =shmat(portArrayId,NULL,0); /*specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo*/
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }


    /*creo la sm per fare il report*/
    reportId = shmget(keyReport,sizeof(report) ,IPC_CREAT | 0666);
    if(reportId == -1){
        printf("errore durante la creazione della memoria condivisa report");
        perror(strerror(errno));
    }

    report =shmat(reportId,NULL,0); /*specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo*/
    if (report == (void *) -1){
        printf("errore durante l'attach della memoria condivisa report durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }


/*creo la fifo perricevere le info di cosa fanno i processi*/


    if (stat("fifo_name1", &st) != 0)
        mkfifo("fifo_name1", 0666);
    else{
        unlink("fifo_name1");
        mkfifo("fifo_name1", 0666);
    }

    semPortArrayId=  semget(keySemPortArray,SO_PORTI,IPC_CREAT | 0666); /*creo semafori della sh*/
    if(semPortArrayId == -1){
        printf("errore durante la creazione dei semafori sh");
        perror(strerror(errno));
    }
    for( i=0;i<SO_PORTI;i++){
        initSemAvailable(semPortArrayId,i);
    }

    semMessageQueueId=  semget(keySemMessageQueue,SO_PORTI,IPC_CREAT | 0666); /*creo semafori della coda di messaggi*/
    if(semMessageQueueId == -1){
        printf("errore durante la creazione dei semafori message");
        perror(strerror(errno));
    }


    semDaysId=  semget(keyGiorni,(SO_PORTI+SO_NAVI),IPC_CREAT | 0666); /*creo semafori gestione giorni*/
    if(semDaysId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }

    semPartiId=  semget(keyStart,1,IPC_CREAT | 0666); /*creo semaforo per far partire i giorni*/
    if(semPartiId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }



    messageQueueId=msgget(keyMessageQueue, IPC_CREAT | 0666); /*creo coda di messaggi*/

    if(messageQueueId == -1){
        printf("errore durante la creazione della coda messaggi");
        perror(strerror(errno));
    }
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

                printf("\nsemaforo della memoria non trovato");
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


   /* shmdt(mem);
    shmdt(memo);*/
    }
    /* shmctl(reportId, IPC_RMID, NULL);*/
    if (msgctl(messageQueueId, IPC_RMID, NULL)== -1) { /*cancella coda di messaggi*/
        if (errno == EINVAL)
            printf("CODA MESSAGGI non trovato");
        else
        TEST_ERROR
    }

    /* shmctl(portArrayId, IPC_RMID,0);
     shmctl(reportId, IPC_RMID,0);*/
    /*azzero semafori dei giorni*/

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
}

int stampaStatoMemoria() {
    struct shmid_ds buf;

    if (shmctl(portArrayId,IPC_STAT,&buf)==-1) {
        fprintf(stderr, "%s: %d. Errore in shmctl #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
        return -1;
    } else {
        printf("\nSTATISTICHE\n");
        printf("AreaId: %d\n",portArrayId);
        printf("Dimensione: %ld\n",buf.shm_segsz);
        printf("Ultima shmat: %s\n",ctime(&buf.shm_atime));
        printf("Ultima shmdt: %s\n",ctime(&buf.shm_dtime));
        printf("Ultimo processo shmat/shmdt: %d\n",buf.shm_lpid);
        printf("Processi connessi: %ld\n",buf.shm_nattch);
        printf("\n");
        printf("\nRead to memory succesful--\n");

        return 0;
    }
}

void reportGiornaliero(){

    int  s2c, c=0;
    char buf[1024], *ptr;
    int sep=0;
    char delim[] = "|";
    char fifo_name1[] = "/tmp/fifo";
    int sellbuy=0;
    int saltaporto=1;
    s2c= open(fifo_name1, O_RDONLY );

    /* receive messages*/
    while (c<SO_MERCI * SO_PORTI) {

        if (read(s2c, &buf, sizeof(char) * 25) > 0) {
            printf("\nRicevo il buf: '%s' ", buf);
            ptr = strtok(buf, delim);
            sep=0;
            sep++;
            if(saltaporto==SO_MERCI){
                saltaporto=0;
                sellbuy=1;
            }
            while (ptr != NULL) {
                if (sep == 1) {
                    printf("PORTO NUMERO: '%s' ", ptr);
                } else if (sep == 2) {
                    printf("Merce numero: '%s' : ", ptr);
                } else if (sep == 3) {
                    printf("In quantita' pari a  '%s' tonnellate ", ptr);
                } else if (sep == 4) {
                    printf(" e' richieta/offerta/non ( '%s' ) ", ptr);
                }else if (sep == 5) {
                    printf(" con '%s' giorni di vita rimanente ", ptr);
                }else if (sep == 6 && sellbuy==1) {
                    printf(" \n Oggi sono state ricevute %s  tonnellate di merce \n", ptr);
                }else if (sep == 7 && sellbuy==1) {
                    printf(" \n Oggi sono state vendute %s  tonnellate di merce \n ", ptr);
                }
                ptr = strtok(NULL, delim);
                sep++;
            }
            printf("\n");
            c++;
            sellbuy=0;
            saltaporto++;
            if (c > SO_MERCI * SO_PORTI)
                break;
        }

    }
    printf("client exit successfully");
    close(s2c);
}


int main() {
    int best = 0, migliore = 0;
    int i, child_pid, status, l, a, fermaPorto, totalePorto;
    char *argv[] = {NULL}, *command = "";


    createIPCKeys();

    fillAndCreate_resource(); /* istanzia tutte le varie code,semafori,memorie condivise necessarie PER TUTTI i processi(keyword static)*/

    clean();
    sleep(1);
    fillAndCreate_resource();


    /*creazione processi porto*/
    for (i = 0; i < SO_PORTI; i++) {
        sleep((unsigned int) 0.60);
        switch (fork()) {
            case 0:
                /* Handle error */

                command = "./porto";
                if (execvp(command, argv) == -1) {
                    printf("errore durante l'esecuzione del execve per il porto \n");
                    perror(strerror(errno));
                }
                exit(EXIT_FAILURE);
            case -1:
                /*padre*/
                break;
            default:
                break;
        }

    }
    sleep(SO_NAVI * 0.55);
    for (i = 0; i < SO_NAVI; i++) {
        sleep((unsigned int) 0.01);
        switch (fork()) {
            case 0:
                /* Handle error */

                command = "./nave";
                if (execvp(command, argv) == -1) {
                    printf("errore durante l'esecuzione del execve per la nave \n");
                    perror(strerror(errno));
                }
                exit(EXIT_FAILURE);
            case -1:

                /*padre*/
                exit(EXIT_FAILURE);
                break;

            default:
                break;
        }

    }

    switch (fork()) {
        case 0:
            /* Handle error */

            command = "./meteo";
            if (execvp(command, argv) == -1) {
                printf("errore durante l'esecuzione del execve per il processo meteo \n");
                perror(strerror(errno));
            }
            exit(EXIT_FAILURE);

        case -1:
            /*padre*/
            exit(EXIT_FAILURE);
            break;

        default:
            break;
    }

    /* appena un figlio termina eseguo altre operazioni */
    while ((child_pid = wait(&status)) != -1) {
        printf("PARENT: PID=%d. Got info of child with PID=%d, status=0x%04X\n", getpid(), child_pid, status);
        if (child_pid > (getpid() + SO_PORTI)) {
            struct sembuf sops;
            sops.sem_num = child_pid - SO_PORTI - getpid();
            sops.sem_op = SO_DAYS ;
            sops.sem_flg = 0;
            if (semop(semDaysId, &sops, 1) == -1) {
                TEST_ERROR
            }
            /*   printf("incrementato semaforo giorni della nave %d a SO_DAYS-1",child_pid);*/
            /*  printf("valore semaforo semDays per la nave %d: %d",child_pid,semctl(semDaysId, sops.sem_num, GETVAL));*/
        } else {
            struct sembuf sops;
            sops.sem_num = child_pid - getpid();
            sops.sem_op = SO_DAYS ;
            sops.sem_flg = 0;
            if (semop(semDaysId, &sops, 1) == -1) {
                TEST_ERROR
            }
            /*  printf("incrementato semaforo giorni del porto %d a SO_DAYS-1",child_pid);*/
        }
        processiMorti++;
    }

    if(processiMorti==(SO_NAVI+SO_PORTI+1)){
    printf("\nSIMULAZIONE TERMINATA\n");

    printf("\n\n<==============================>\n Durante la simulazione sono state:\n Rallentate %d Navi\n Rallentati %d Porti\n Affondate %d Navi\n<==============================>\n\n",
           report->rallentate, report->rallentati, report->affondate);


    printf("\n\nCi sono %d navi in mare senza carico\nCi sono %d navi in mare con carico\nCi sono %d navi a commerciare ai porti\n",
           report->senzaCarico, report->conCarico, report->inPorto);

    for (l = 0; l < SO_PORTI; l++) {
        totalePorto = 0;
        for (a = 0; a < SO_MERCI; a++) {
            if (portArrays[l].merce[a].offertaDomanda == 1)
                totalePorto += portArrays[l].merce[a].quantita;
        }
        printf("\n\n Nel porto %d sono:\n -presenti %d tonnellate di merce\n -state ricevute %d tonnellate di merce\n -state spedite %d tonnellate di merce\n",
               l, totalePorto, report->ricevutePorto[l], report->speditePorto[l]);
    }


    for (l = 0; l < SO_MERCI; l++) {
        fermaPorto = 0;
        for (a = 0; a < SO_PORTI; a++) {
            if (portArrays[a].merce[l].offertaDomanda == 1)
                fermaPorto += portArrays[a].merce[l].quantita;
        }
        printf("\n\nLa merce %d è:\n -stata generata all'inizio una quantità pari a %d tonnellate\n -rimasta ferma nei porti in quantità pari a %d (se maggiore è causata dall'arrivo in porto di nuova merce non per via marittima)\n -scaduta nei porti in quantità pari a %d\n -scaduta nelle navi in quantità pari a %d\n -consegnata da qualche nave in quantità pari a %d tonnellate \n",
               l, report->merciGenerate[l], fermaPorto, report->merciScadutePorto[l], report->merciScaduteNave[l],
               report->consegnataDaNave[l]);
    }


    for (l = 0; l < SO_PORTI; l++) {
        if (report->offerte[l] > best) {
            best = report->offerte[l];
            migliore = l;
        }
    }
    printf("\nIl porto con la miglior offerta e' stato %d\n", migliore);
    best = 0;
    migliore = 0;
    for (l = 0; l < SO_PORTI; l++) {
        if (report->richieste[l] > best) {
            best = report->richieste[l];
            migliore = l;
        }
    }
    printf("\nIl porto con la maggior richiesta è stato %d", migliore);




    clean();

}
}