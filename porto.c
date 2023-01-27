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
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>



#include "utility.h"
#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }
int chiudo=0;
float ricevutaOggi=0;
float speditaOggi=0;
int giorniSimulazione = 0, idSemBanchine, indicePorto;
int banchineOccupate=0;
int merceScaduta=0;
int  scadute[SO_MERCI];
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
}


void comunicazioneNave(int numSemBanchina) {
    /*buf = malloc(sizeof(struct msgbuf)); */
    int sep = 0;
    struct msgbuf buf;
    char msg[15 * SO_MERCI];

    int id;

    if ((messageQueueId = msgget(keyMessageQueue, 0)) == -1) {
        perror("client: Failed to create message queue:");
        exit(2);
    }
    if ((msgrcv(messageQueueId, &buf, sizeof(buf.mText), getpid(), IPC_NOWAIT)) == -1) {
        if(errno==ENOMSG){
            printf("non è stato trovato il messaggio richiesto, perso un messaggio in una banchina, porto.c\n");
        }
        TEST_ERROR;
    } else {
        banchineOccupate+=1;

    }
    char messaggio[30 * SO_MERCI];
    char workString[30]; /*string temporanea per poi scrivere tutto il messaggio */
    char delim[] = "|";
    int scadenza=0;
    int quantitaAttuale = 0;
    int ron = 2;/*richiesta offerta non */
    int nomeMerceChiesta = 0;
    char *ptr = strtok(buf.mText, delim);
    char tmp[20];
    int pidAsked = 0;
    sep = 0;
    sep++;
    while (ptr != NULL) {
        if (sep == 1) {
            strcpy(tmp, ptr);
            pidAsked = atoi(tmp);
        } else if (sep == 2) {

            strcpy(tmp, ptr);
            nomeMerceChiesta = atoi(tmp);
        } else if (sep == 3) {
            strcpy(tmp, ptr);
            ron = atoi(tmp);
            if (ron == 0){}
            else if (ron == 1){}
            /* printf(" e' in vendita "); */
            /*else */
            /* printf(" non è di interesse della nave"); */
        } else if (sep == 4) {
            strcpy(tmp, ptr);
            quantitaAttuale = atoi(tmp);

            /*quando arrivo al separatore 4 effettuo i primi scambi se necessari */

            if (ron == 0 || ron == 1) { /*inizia la trattativa */
                if (ron == 0 && portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda ==1) { /*se la nave vuole e porto vende: */
                    if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita >= quantitaAttuale) {
                        portArrays[indicePorto].merce[nomeMerceChiesta].quantita =portArrays[indicePorto].merce[nomeMerceChiesta].quantita - quantitaAttuale;
                        speditaOggi += quantitaAttuale; /*tiene conto delle merci vendute oggi */
                        if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita == 0)
                            portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda = 2;
                        ron = 1;

                    } else if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita < quantitaAttuale) {
                        int differenza=quantitaAttuale-portArrays[indicePorto].merce[nomeMerceChiesta].quantita;
                        quantitaAttuale =differenza;
                        speditaOggi += quantitaAttuale;
                        portArrays[indicePorto].merce[nomeMerceChiesta].quantita = 0;
                        portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda = 2;
                    }

                } else if (ron == 1 && portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda ==0) { /*se la nave vende e porto compra: */
                    if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita >= quantitaAttuale) {
                        portArrays[indicePorto].merce[nomeMerceChiesta].quantita = quantitaAttuale;
                        ricevutaOggi += quantitaAttuale; /*tiene conto delle merci comprate oggi */
                        quantitaAttuale = 0;
                        portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda = 2;
                        ron = 2;
                    } else if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita < quantitaAttuale) {
                        int differenza=quantitaAttuale- portArrays[indicePorto].merce[nomeMerceChiesta].quantita;
                        portArrays[indicePorto].merce[nomeMerceChiesta].quantita=quantitaAttuale-differenza;
                        quantitaAttuale =differenza;
                        portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda = 1;
                        ricevutaOggi += portArrays[indicePorto].merce[nomeMerceChiesta].quantita;
                    }
                }
                if(ron==1)
                    scadenza=portArrays[indicePorto].merce[nomeMerceChiesta].vitaMerce;
                if(ron==2)
                    scadenza=0;
            }

            sprintf(workString, "%d|%d|%d|%d|%d|", getpid(), nomeMerceChiesta, ron, quantitaAttuale,scadenza);
            strcat(messaggio, workString);

            strcpy(workString,"");
            sep=0;
        }

        ptr = strtok(NULL, delim);
        sep++;
    }
    struct msgbuf buf1;

    buf1.mType = pidAsked;

    strcpy(buf1.mText, messaggio);

    if ((msgsnd(messageQueueId, &buf1, sizeof(buf1.mText), 0)) == -1) {
        TEST_ERROR;
    } else {
        /*incrementare semaforo a 3 */
        while(semctl(portArrays[indicePorto].semIdBanchinePorto,numSemBanchina,GETVAL) != 3){
            if (releaseSem(portArrays[indicePorto].semIdBanchinePorto, numSemBanchina) == -1) {
                printf("errore durante l'incremento del semaforo per scrivere sulla coda di messaggi ");
                TEST_ERROR;
            }
        }
    }
    /* Porto finisce la trattazione */



}

void findScambi(){
    for(int i=0;i<SO_BANCHINE;i++){
        if(semctl(portArrays[indicePorto].semIdBanchinePorto,i,GETVAL) == 2){
            comunicazioneNave(i);
        }
    }
}


/*funzione che riempe l'array di struct dei porti */
void setPorto(){
    while(portArrays[indicePorto].idPorto!=0){
        indicePorto++;
    }
    int numSem;
    semPortArrayId = semget(keySemPortArray,SO_PORTI-1,0);
    for(int i=0;i<SO_PORTI;i++){
        numSem = semctl(semPortArrayId,0,GETVAL);
        if(numSem == -1){
            TEST_ERROR;
        }
        if(numSem == 1){
            if(reserveSem( semPortArrayId, i)==-1){ /*richiede la memoria e la occupa SOLO LUI */
                printf("errore durante il decremento del semaforo per inizializzare il porto");
                perror(strerror(errno));
            }
            break;
        }
    }
    if(portArrays[indicePorto].idPorto==0){
        portArrays[indicePorto].idPorto=getpid();
        portArrays[indicePorto].semIdBanchinePorto = semget(IPC_PRIVATE,SO_BANCHINE,IPC_CREAT | 0600);
        for(int j=0;j<SO_BANCHINE-1;j++){
            initSemAvailable(portArrays[indicePorto].semIdBanchinePorto,j);
        }
        if(indicePorto==0){ /*inizializza porto */
            portArrays[indicePorto].x=0;
            portArrays[indicePorto].y=0;
        }else if(indicePorto==1){
            portArrays[indicePorto].x=SO_LATO;
            portArrays[indicePorto].y=0;
        }else if(indicePorto==2){
            portArrays[indicePorto].x=SO_LATO;
            portArrays[indicePorto].y=SO_LATO;
        }else if(indicePorto==3){
            portArrays[indicePorto].x=0;
            portArrays[indicePorto].y=SO_LATO;
        }else {
            srand(getpid());
            portArrays[indicePorto].x=(rand() %  (int)SO_LATO+1);
            portArrays[indicePorto].y=(rand() %  (int)SO_LATO+1);
            for(int j=0;j<indicePorto-1;j++){ /*controllo che non si crei sulla posizione di un altro porto */
                if(portArrays[indicePorto].x== portArrays[j].x && portArrays[indicePorto].y==portArrays[j].y){
                    j=-1;
                    portArrays[indicePorto].x=(rand() %  (int)SO_LATO+1);
                    portArrays[indicePorto].y=(rand() %  (int)SO_LATO+1);
                }

            }
        }

    }
    for(int k=0;k<SO_MERCI;k++){
        srand(time(NULL));
        portArrays[indicePorto].merce[k].nomeMerce = k;
        portArrays[indicePorto].merce[k].offertaDomanda = (rand() %  2);/*0 = domanda, 1 = offerta, 2 = da assegnare */
        if(portArrays[indicePorto].merce[k].offertaDomanda ==1)
            portArrays[indicePorto].merce[k].vitaMerce = (SO_MIN_VITA + (rand() %  (SO_MAX_VITA-SO_MIN_VITA))); /*giorni di vita */

    }

}

void setMerci(){
    srand(getpid());
    for(int j=0;j<SO_MERCI;j++){

        portArrays[indicePorto].merce[j].nomeMerce = j;
        portArrays[indicePorto].merce[j].offertaDomanda = (rand() %  2);/*0 = domanda, 1 = offerta, 2 = da assegnare */
        if(portArrays[indicePorto].merce[j].offertaDomanda ==1)
            portArrays[indicePorto].merce[j].vitaMerce = (SO_MIN_VITA + (rand() %  (SO_MAX_VITA-SO_MIN_VITA))); /*giorni di vita */
        else
            portArrays[indicePorto].merce[j].vitaMerce =0;

        if(indicePorto==0&&j==0){
            portArrays[indicePorto].merce[j].quantita=(rand() %  ((SO_SIZE/2)/SO_MERCI));

        }else  if ( j==SO_MERCI-1)
            portArrays[indicePorto].merce[j].quantita=(SO_SIZE/2)-sum;
        else {
            portArrays[indicePorto].merce[j].quantita=( rand() % ((SO_SIZE/2)-sum)/(SO_MERCI-j) );

        }

        if(portArrays[indicePorto].merce[j].quantita>0)
            sum+=portArrays[indicePorto].merce[j].quantita;

        if(sum>SO_SIZE)
            portArrays[indicePorto].merce[j].quantita=0;

        if(portArrays[indicePorto].merce[j].quantita<0 )
            portArrays[indicePorto].merce[j].quantita=0;

        if(portArrays[indicePorto].merce[j].quantita==0)
            portArrays[indicePorto].merce[j].offertaDomanda=2;

        if(portArrays[indicePorto].merce[j].offertaDomanda==2)
            portArrays[indicePorto].merce[j].vitaMerce =0;

    }
}


void spawnMerci(){
    int merceSpawnata;
    srand(getpid());
    int totale;

    for(int a=0;a<SO_MERCI;a++){
        totale=portArrays[indicePorto].merce[a].quantita;
    }

    int j = rand() % (501+SO_MERCI);
    while(j>SO_MERCI)
        j-=SO_MERCI;
    if(totale+(SO_SIZE / SO_DAYS)<SO_SIZE){/*CONTROLLA CHE IL TOTALE NON VENGA MAI SUPERATO */
        if (portArrays[indicePorto].merce[j].offertaDomanda == 1 || portArrays[indicePorto].merce[j].offertaDomanda == 2) {/*0 = domanda, 1 = offerta, 2 = da assegnare */
            if (portArrays[indicePorto].merce[j].offertaDomanda == 2) {
                portArrays[indicePorto].merce[j].vitaMerce = (SO_MIN_VITA + (rand() % (SO_MAX_VITA - SO_MIN_VITA)));  /*giorni di vita */

                portArrays[indicePorto].merce[j].quantita = ((SO_SIZE / SO_DAYS) );


            } else if (portArrays[indicePorto].merce[j].offertaDomanda == 1) {
                portArrays[indicePorto].merce[j].quantita += ((SO_SIZE / SO_DAYS) );


            }


        }}


}
void gestioneInvecchiamentoMerci(){ /*funzione da richiamare ogni "giorno" di simulazione per controllare se la merce del porto è scaduta */

    for(int k=0;k<SO_MERCI;k++){
        if(portArrays[indicePorto].merce[k].offertaDomanda==1){
            if(portArrays[indicePorto].merce[k].vitaMerce <=0&&portArrays[indicePorto].merce[k].offertaDomanda==1){ /*decidere se cancellare o inizializzare */
                portArrays[indicePorto].merce[k].offertaDomanda=2;
                portArrays[indicePorto].merce[k].vitaMerce=0;
                merceScaduta+= portArrays[indicePorto].merce[k].quantita;
                scadute[k]+= portArrays[indicePorto].merce[k].quantita;
                portArrays[indicePorto].merce[k].quantita=0;
            }
            else{
                portArrays[indicePorto].merce[k].vitaMerce-=1;
            }
        }
    }
}


int stampaStatoMemoriaa() {

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

void reportGiornalieroPorto(){
    float dormi=((SO_MERCI*(0.062)));
    int s2c, c2s, i;
    char fifo_name1[] = "/tmp/fifo";
    int k=0;
    char messaggio[80], buf[1024];
    struct stat st;
    /* se non esiste la fifo, la creo */
    if (stat(fifo_name1, &st) != 0)
        mkfifo(fifo_name1, 0666);


    s2c=open(fifo_name1, O_WRONLY);
    for (int j=0; j<SO_MERCI; j++)
    {
        sprintf(messaggio,"%d|%d|%d|%d|%d|%d|%d|",indicePorto,portArrays[indicePorto].merce[j].nomeMerce,(int)portArrays[indicePorto].merce[j].quantita,portArrays[indicePorto].merce[j].offertaDomanda,portArrays[indicePorto].merce[j].vitaMerce,(int)ricevutaOggi,(int)speditaOggi);
        strcpy(messaggio, messaggio);
        write(s2c, messaggio, strlen(messaggio)+1);

        if(dormi>=0)
            sleep((dormi));
    }

    close(c2s);
    chiudo=1;
}


void checkUtilita(){/*funzione che vede se il porto deve fare ancora qualcosa (vende/comprare),se no uccide il processo */
    int morto=1;
    int k=0;
    while(portArrays[k].idPorto!=getpid() && k<=SO_PORTI)
        k++;
    for(int q=0;q<SO_MERCI;q++){
        if(portArrays[k].merce[q].offertaDomanda!=2){
            morto=0;
        }
    }
    if(morto==1)
        kill(getpid(),SIGSEGV);
}

void handle_signal(int signum) {
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    char str[10];
    if(SO_SWELL_DURATION<10)
        sprintf(str,"%d",SO_SWELL_DURATION*10);
    else
        sprintf(str,"%d",SO_STORM_DURATION);
    strcat(str,"000000L");
    tim.tv_nsec = atoi(str);
    switch (signum) {
        case SIGINT:
            TEST_ERROR
            break;
        case SIGUSR1:
            nanosleep(&tim,&tim2);
            break;
        default:
            break;
    }
}

void startPorto(int argc, char *argv[]){

    struct sigaction sa;
    sigset_t my_mask;
    sa.sa_handler = &handle_signal;
    sigemptyset(&my_mask); /* do not mask any signal */
    sa.sa_mask = my_mask;
    sigaction(SIGUSR1, &sa, NULL);

    for(int i=0;i<SO_MERCI;i++){
        scadute[i]=0;
    }

    createIPCKeys();
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        TEST_ERROR
    }
    portArrays = shmat(portArrayId,NULL,0);
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        TEST_ERROR
    }
    semDaysId=  semget(keyGiorni,SO_PORTI+SO_NAVI,IPC_CREAT | 0666); /*creo semafori gestione giorni */
    if(semDaysId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }

    semPartiId=  semget(keyStart,1,IPC_CREAT | 0666); /*creo semaforo per far partire i giorni */
    if(semPartiId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }
    setPorto();
    setMerci();

    printf("Mi trovo sul porto n :%d \n",indicePorto);

    printf("X del porto %d: %d   \n",indicePorto,portArrays[indicePorto].x);
    printf("Y del porto %d: %d   \n",indicePorto,portArrays[indicePorto].y);
    printf("ID DEL PORTO :%d \n",portArrays[indicePorto].idPorto);


    for(int j=0;j<SO_MERCI;j++){
        int q=portArrays[indicePorto].merce[j].quantita;

        printf("\n PORTO NUMERO:%d La merce numero %d e' richieta/offerta/non (%d)  in qualita' pari a :%d tonnellate con una vita (se venduta)  di %d giorni \n",indicePorto,portArrays[indicePorto].merce[j].nomeMerce,portArrays[indicePorto].merce[j].offertaDomanda,q,portArrays[indicePorto].merce[j].vitaMerce);

    }


    if ((messageQueueId = msgget(keyMessageQueue, 0)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }

    int quantitaNelPorto=0;
    if(portArrays[SO_PORTI-1].idPorto==0){}
    sleep(0.1*SO_NAVI);

    while(giorniSimulazione<SO_DAYS) {
        int controlloStop = 0;/*se raggiunge so navi termina la sim perchè non abbiamo più navi in circolo */
        for (int j = SO_PORTI; j < SO_PORTI + SO_NAVI; j++)
            if (semctl(semDaysId, j, GETVAL) >= SO_DAYS) {
                controlloStop++;
            }
        if (controlloStop != SO_NAVI) {

            sigaction(SIGUSR1, &sa, NULL);
            banchineOccupate = 0;
            printf("Giorno per porto: %d.\n", giorniSimulazione);

            /*casualmente sceglie se generare merce o no */
            srand(getpid());
            int random = rand() % 2;
            if (random == 0)
                spawnMerci();
            sleep(0.02);
            findScambi();

            printf("\n Oggi sono state vendute %d tonnellate e sono state ricevute %d tonnellate", (int) speditaOggi,
                   (int) ricevutaOggi);

            printf("\n Oggi sono state occupate %d banchine nel porto %d", banchineOccupate,indicePorto);

            for (int j = SO_PORTI; j < SO_PORTI + SO_NAVI; j++)
                while (semctl(semDaysId, j, GETVAL) < giorniSimulazione + 1) {}


            while (semctl(semDaysId, indicePorto, GETVAL) < giorniSimulazione + 1) {
                if (releaseSem(semDaysId, indicePorto) == -1) {
                    printf("errore durante l'incremento del semaforo per incrementare i giorni ");
                    TEST_ERROR;
                }
            }
            giorniSimulazione++;
            gestioneInvecchiamentoMerci();
            printf("\nLa quantità di merce scaduta in porto oggi: %d", merceScaduta);
            quantitaNelPorto = 0;
            for (int j = 0; j < SO_MERCI; j++) {
                int q = portArrays[indicePorto].merce[j].quantita;
                quantitaNelPorto += portArrays[indicePorto].merce[j].quantita;

                if (scadute[j] > 0)
                    printf("\nLa merce %d è scaduta in quantita' pari a :%d ", j,
                           scadute[j]); /*stampa solo se della merce è scaduta */
            }
            printf("\nNel porto ci sono un totale di merci pari a :%d ", quantitaNelPorto);
            int tot = 0;
            if (indicePorto == SO_PORTI - 1) {/*report di tutte le merci */
                for (int j = 0; j < SO_MERCI; j++) {
                    tot = 0;
                    for (int i = 0; i < SO_PORTI; i++) {
                        if (portArrays[i].merce[j].offertaDomanda == 1)
                            tot += portArrays[i].merce[j].quantita;

                    }
                    printf("\n La merce %d è presente in totale su tutti i porti in quantità pari a %d", j, tot);
                }

            }
            sleep(0.2);
            if (giorniSimulazione == SO_DAYS - 1)
                break;
        }else{

            while (semctl(semDaysId, indicePorto, GETVAL) < SO_DAYS) {
                if (releaseSem(semDaysId, indicePorto) == -1) {
                    printf("errore durante l'incremento del semaforo per incrementare i giorni ");
                    TEST_ERROR;
                }
            }
            if(indicePorto==SO_PORTI-1)
                printf("\n\n\n\n\nLA SIMULAZIONE TERMINA PER MANCANZA DI NAVI AL GIORNO %d",giorniSimulazione);
            giorniSimulazione=SO_DAYS;
        }
    }
    printf("\n porto %d muore",indicePorto);
    exit(EXIT_SUCCESS);
}
