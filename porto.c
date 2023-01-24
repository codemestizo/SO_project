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
    //buf = malloc(sizeof(struct msgbuf));
    int sep = 0;
    struct msgbuf buf;
    char msg[15 * SO_MERCI];

    int id;
    printf("dentro comunicazione nave, il porto %d sa che deve ricevere e id coda messaggi %d che riceve", getpid(),messageQueueId);
    //messageQueueId=msgget(keyMessageQueue, IPC_CREAT | 0666); //creo coda di messaggi
    //buf->mType=getpid();
    // while (1) {

    if ((messageQueueId = msgget(keyMessageQueue, 0)) == -1) {
        perror("client: Failed to create message queue:");
        exit(2);
    }
    if ((msgrcv(messageQueueId, &buf, sizeof(buf.mText), getpid(), IPC_NOWAIT)) == -1) { //- sizeof(long)
        TEST_ERROR;
    } else {
        banchineOccupate+=1;

    }
    char messaggio[30 * SO_MERCI];
    char workString[30]; //string temporanea per poi scrivere tutto il messaggio
    char delim[] = "|";
    int scadenza=0;
    int quantitaAttuale = 0;
    int ron = 2;//richiesta offerta non
    int nomeMerceChiesta = 0;
    char *ptr = strtok(buf.mText, delim);
    char tmp[20];
    int pidAsked = 0;
    sep = 0;
    sep++;
    while (ptr != NULL) {
        if (sep == 1) {
            //printf("\nIL PID DELLA NAVE CHE MI HA SCRITTO E': '%s' ", ptr);
            strcpy(tmp, ptr);
            pidAsked = atoi(tmp);
        } else if (sep == 2) {

            strcpy(tmp, ptr);
            nomeMerceChiesta = atoi(tmp);
            //printf("Merce numero: '%d' : ", nomeMerceChiesta);
        } else if (sep == 3) {
            strcpy(tmp, ptr);
            ron = atoi(tmp);
            if (ron == 0){}
               // printf(" e' richieta ");
            else if (ron == 1){}
               // printf(" e' in vendita ");
            //else
               // printf(" non è di interesse della nave");
        } else if (sep == 4) {
            strcpy(tmp, ptr);
            quantitaAttuale = atoi(tmp);
           // printf(" in '%d' tonnellate ", quantitaAttuale);

            //quando arrivo al separatore 4 effettuo i primi scambi se necessari
            //indicePorto = 0;

            if (ron == 0 || ron == 1) { //inizia la trattativa
              //  printf("ron10");
                if (ron == 0 && portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda ==1) { //se la nave vuole e porto vende:
                    //printf("ron0");
                    if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita >= quantitaAttuale) {
                        //printf("\nIl porto ne ha %d e la nave ne vuole %d ",portArrays[indicePorto].merce[nomeMerceChiesta].quantita,quantitaAttuale);//controllo tattico
                        portArrays[indicePorto].merce[nomeMerceChiesta].quantita =portArrays[indicePorto].merce[nomeMerceChiesta].quantita - quantitaAttuale;
                        speditaOggi += quantitaAttuale; //tiene conto delle merci vendute oggi
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

                } else if (ron == 1 && portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda ==0) { //se la nave vende e porto compra:
                   // printf("ron1");
                    if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita >= quantitaAttuale) {
                        portArrays[indicePorto].merce[nomeMerceChiesta].quantita = quantitaAttuale;
                        ricevutaOggi += quantitaAttuale; //tiene conto delle merci comprate oggi
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

    printf("\n MESSAGGIO %s",messaggio);

    strcpy(buf1.mText, messaggio);

    if ((msgsnd(messageQueueId, &buf1, sizeof(buf1.mText), 0)) == -1) {
        TEST_ERROR;
    } else {
        printf("messaggio spedito da porto.c %s  ",buf1.mText);
        //settare semaforo a 3
        while(semctl(portArrays[indicePorto].semIdBanchinePorto,numSemBanchina,GETVAL) != 3){
            if (releaseSem(portArrays[indicePorto].semIdBanchinePorto, numSemBanchina) == -1) {
                printf("errore durante l'incremento del semaforo per scrivere sulla coda di messaggi ");
                TEST_ERROR;
            }
            //printf("\nsemaforo di merda  valore %d\n, idSemaforo %d",semctl(portArrays[indicePorto].semIdBanchinePorto,numSemBanchina,GETVAL),portArrays[indicePorto].semIdBanchinePorto);

        }
       // printf("\nporto, incremento il semaforo banchina per porto, idSemBanchine: %d\n numSemBanchina valore: %d  valore %d\n",portArrays[indicePorto].semIdBanchinePorto,numSemBanchina,semctl(portArrays[indicePorto].semIdBanchinePorto,numSemBanchina,GETVAL));
    }
   // printf("\nPorto finisce la trattazione\n");



}

void findScambi(){
    //printf("entro in findScambi\n");
    int numSem;
    for(int i=0;i<SO_BANCHINE;i++){
       // printf("idBanchinePorto ottenuto in findScambi: %d \nindice banchina ottenuto in findScambi: %d \n",portArrays[indicePorto].semIdBanchinePorto,i);

        if(semctl(portArrays[indicePorto].semIdBanchinePorto,i,GETVAL) == 2){//numSem == 0
           // printf("Sto per entrare in cm porto\n");

            comunicazioneNave(i);
        }

    }
   // printf("esco");
}




//funzione che riempirà le struct dei porti
void setPorto(){
    while(portArrays[indicePorto].idPorto!=0){
        indicePorto++;
    }
    int numSem;
    semPortArrayId = semget(keySemPortArray,SO_PORTI-1,0);
    //printf("semPortArrayId: %d \n", semPortArrayId);
    for(int i=0;i<SO_PORTI;i++){
        numSem = semctl(semPortArrayId,0,GETVAL);
        if(numSem == -1){
            TEST_ERROR;
        }
        if(numSem == 1){
            if(reserveSem( semPortArrayId, i)==-1){ //richiede la memoria e la occupa SOLO LUI
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
        if(indicePorto==0){ //set spawn porto
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
            for(int j=0;j<indicePorto-1;j++){ //controllo che non spawni sulla posizione di un altro porto
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
        portArrays[indicePorto].merce[k].offertaDomanda = (rand() %  2);//0 = domanda, 1 = offerta, 2 = da assegnare
        if(portArrays[indicePorto].merce[k].offertaDomanda ==1)
            portArrays[indicePorto].merce[k].vitaMerce = (SO_MIN_VITA + (rand() %  (SO_MAX_VITA-SO_MIN_VITA))); //giorni di vita

    }

}

void setMerci(){
    srand(getpid());
    for(int j=0;j<SO_MERCI;j++){

        portArrays[indicePorto].merce[j].nomeMerce = j;
        portArrays[indicePorto].merce[j].offertaDomanda = (rand() %  2);//0 = domanda, 1 = offerta, 2 = da assegnare
        if(portArrays[indicePorto].merce[j].offertaDomanda ==1)
            portArrays[indicePorto].merce[j].vitaMerce = (SO_MIN_VITA + (rand() %  (SO_MAX_VITA-SO_MIN_VITA))); //giorni di vita
        else
            portArrays[indicePorto].merce[j].vitaMerce =0;

        if(indicePorto==0&&j==0){
            portArrays[indicePorto].merce[j].quantita=(rand() %  (SO_SIZE/SO_MERCI));

        }else  if ( j==SO_MERCI-1)
            portArrays[indicePorto].merce[j].quantita=SO_SIZE-sum;//(SO_FILL-sum)
        else {
            portArrays[indicePorto].merce[j].quantita=( rand() % (SO_SIZE-sum)/(SO_MERCI-j) );

        }

        if(portArrays[indicePorto].merce[j].quantita>0)
            sum+=portArrays[indicePorto].merce[j].quantita;

        if(sum>SO_SIZE)
            portArrays[indicePorto].merce[j].quantita=0;// portArrays[i].merce[j].quantita-=(sum-SO_FILL);

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
    int k = rand() % SO_MERCI; //quante merci arriveranno quel giorno
    for(int h=0;h<k;h++) {
        int j = rand() % SO_MERCI;
        while(h>0 && merceSpawnata==j)
            j = rand() % SO_MERCI; //per evitare di spawnare più merce dello stesso tipo
         merceSpawnata=j;

        if (portArrays[indicePorto].merce[j].offertaDomanda == 1 || portArrays[indicePorto].merce[j].offertaDomanda == 2) {//0 = domanda, 1 = offerta, 2 = da assegnare
            if (portArrays[indicePorto].merce[j].offertaDomanda == 2) {
                portArrays[indicePorto].merce[j].vitaMerce = (SO_MIN_VITA + (rand() % (SO_MAX_VITA - SO_MIN_VITA)));  //giorni di vita

                portArrays[indicePorto].merce[j].quantita = ((SO_SIZE / SO_DAYS) /  k);       //k sarà quante merci arriveranno


            } else if (portArrays[indicePorto].merce[j].offertaDomanda == 1) {
                portArrays[indicePorto].merce[j].quantita += ((SO_SIZE / SO_DAYS) / k);       //k sarà quante merci arriveranno


            }


        }

    }
}
void gestioneInvecchiamentoMerci(){ //funzione da richiamare ogni "giorno" di simulazione per checkare se la merce del porto è scaduta

        for(int k=0;k<SO_MERCI;k++){
            if(portArrays[indicePorto].merce[k].offertaDomanda==1){
            if(portArrays[indicePorto].merce[k].vitaMerce <=0&&portArrays[indicePorto].merce[k].offertaDomanda==1){ //decidere se cancellare proprio o settare a 0 e da assegnare il tutto
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
        // printf("x del primo porto: %d",portArrays[0].x);
        printf("Ultimo processo shmat/shmdt: %d\n",buf.shm_lpid);
        printf("Processi connessi: %ld\n",buf.shm_nattch);
        printf("\n");



        printf("\nRead to memory succesful--\n");

        return 0;
    }
}

void reportGiornalieroPorto(){
    float dormi=((SO_MERCI*(0.062))); //0.05 senza spediti e ricevuti , 0.062 con
    int s2c, c2s, i;
    char fifo_name1[] = "/tmp/fifo";
    int k=0;
    char messaggio[80], buf[1024];
    struct stat st;
    // if no fifos, create 'em
    if (stat(fifo_name1, &st) != 0)
        mkfifo(fifo_name1, 0666);


    s2c=open(fifo_name1, O_WRONLY);
    for (int j=0; j<SO_MERCI; j++)
    {
        //sprintf(messaggio," %d|%d|%d|%d|%d|%d/",k,4,5,6,5,5);
        sprintf(messaggio,"%d|%d|%d|%d|%d|%d|%d|",indicePorto,portArrays[indicePorto].merce[j].nomeMerce,(int)portArrays[indicePorto].merce[j].quantita,portArrays[indicePorto].merce[j].offertaDomanda,portArrays[indicePorto].merce[j].vitaMerce,(int)ricevutaOggi,(int)speditaOggi); //,(int)ricevutaOggi,(int)speditaOggi) //portArrays[k].merce[i].nomeMerce,portArrays[k].merce[i].quantita,portArrays[k].merce[i].offertaDomanda,ricevutaOggi,speditaOggi
        strcpy(messaggio, messaggio);
        write(s2c, messaggio, strlen(messaggio)+1);

        if(dormi>=0)
            sleep((dormi));
    }


    // delete fifos
    // unlink(fifo_name1); //non la decommentare o rompi tutto
    close(c2s);
    chiudo=1;
}


void checkUtilita(){//funzione che vede se il porto deve fare ancora qualcosa (vende/comprare),se no uccide il processo
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

void startPorto(int argc, char *argv[]){
    //printf(" \n PID DI STO PROCESSO %d",getpid());
    //printf("keyPortArray %d \n",keyPortArray);
    for(int i=0;i<SO_MERCI;i++){
        scadute[i]=0;
    }

    //keyPortArray = ftok("master.c", 'u');
    createIPCKeys();
    int size = (sizeof(portDefinition) + (sizeof(structMerce) * SO_MERCI)) * SO_PORTI;
    portArrayId = shmget(keyPortArray,size,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa portArray");
        TEST_ERROR
    }
    portArrays = shmat(portArrayId,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        TEST_ERROR
    }
    semDaysId=  semget(keyGiorni,SO_PORTI+SO_NAVI,IPC_CREAT | 0666); //creo semafori gestione giorni
    if(semDaysId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }

    semPartiId=  semget(keyStart,1,IPC_CREAT | 0666); //creo semaforo per far partire i giorni
    if(semPartiId == -1){
        printf("errore durante la creazione dei semafori giorni");
        perror(strerror(errno));
    }
    setPorto();
    //sleep(k);
    setMerci();

    printf("Mi trovo sul porto n :%d \n",indicePorto);
    //stampaStatoMemoriaa();

    printf("X del porto %d: %d   \n",indicePorto,portArrays[indicePorto].x);
    printf("Y del porto %d: %d   \n",indicePorto,portArrays[indicePorto].y);
    printf("ID DEL PORTO :%d \n",portArrays[indicePorto].idPorto);


    for(int j=0;j<SO_MERCI;j++){
        int q=portArrays[indicePorto].merce[j].quantita;

        printf("\n PORTO NUMERO:%d La merce numero %d e' richieta/offerta/non (%d)  in qualita' pari a :%d tonnellate con una vita (se venduta)  di %d giorni \n",indicePorto,portArrays[indicePorto].merce[j].nomeMerce,portArrays[indicePorto].merce[j].offertaDomanda,q,portArrays[indicePorto].merce[j].vitaMerce);//,portArrays[k].merce[j].quantita

    }
    // reportGiornalieroPorto();
    //
    //}


    if ((messageQueueId = msgget(keyMessageQueue, 0)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }
    //printf("message queue: ready to receive messages.\n");

    int quantitaNelPorto=0;
    if(portArrays[SO_PORTI-1].idPorto==0){}
    sleep(0.1*SO_NAVI);

    while(giorniSimulazione<SO_DAYS){
        banchineOccupate=0;
        printf("Giorno per porto: %d.\n",giorniSimulazione);

        //randomicamente sceglie se generare merce o no
        srand(getpid());
        int random=rand()%2;
        //if(random==0)
            spawnMerci();
        sleep(0.02);
        findScambi();
        //printf("allora in sto porto vedo il giorno %d",semctl(semDaysId,indicePorto,GETVAL));

        printf("\n Oggi sono state vendute %d tonnellate e sono state ricevute %d tonnellate",(int)speditaOggi,(int)ricevutaOggi);

        printf("\n Oggi sono state occupate %d banchine",banchineOccupate);

        for(int j=SO_PORTI;j<SO_PORTI+SO_NAVI;j++)
        while (semctl(semDaysId, j, GETVAL) != giorniSimulazione + 1) {}
        while(semctl(semDaysId,indicePorto,GETVAL) < giorniSimulazione+1){
            if (releaseSem(semDaysId, indicePorto) == -1) {
                printf("errore durante l'incremento del semaforo per incrementare i giorni ");
                TEST_ERROR;
            }
        }
        //printf("Giorno incrementato %d per porto: %d.\n",semctl(semDaysId,indicePorto,GETVAL),indicePorto);
        giorniSimulazione++;
       // if(giorniSimulazione!=SO_DAYS-1) {
            //while (semctl(semPartiId, 0, GETVAL) != giorniSimulazione + 1) {}

      //  }
        gestioneInvecchiamentoMerci();
        printf("\nLa quantità di merce scaduta in porto oggi: %d",merceScaduta);
         quantitaNelPorto=0;
        for(int j=0;j<SO_MERCI;j++){
            int q=portArrays[indicePorto].merce[j].quantita;
            quantitaNelPorto+=portArrays[indicePorto].merce[j].quantita;
            printf("\n PORTO NUMERO:%d La merce numero %d e' richieta/offerta/non (%d)  in qualita' pari a :%d tonnellate con una vita (se venduta)  di %d giorni \n",indicePorto,portArrays[indicePorto].merce[j].nomeMerce,portArrays[indicePorto].merce[j].offertaDomanda,q,portArrays[indicePorto].merce[j].vitaMerce);//,portArrays[k].merce[j].quantita
            if(scadute[j]>0)
            printf("\nLa merce %d è scaduta in quantita' pari a :%d ",j,scadute[j]); //printa solo se della merce è scaduta
        }
        printf("\nNel porto ci sono un totale di merci pari a :%d ",quantitaNelPorto);
        int tot=0;
        if(indicePorto==SO_PORTI-1){//report di tutte le merci
            for (int j= 0; j<SO_MERCI;j++) {
                tot=0;
               for (int i= 0; i<SO_PORTI;i++) {
                if(portArrays[i].merce[j].offertaDomanda==1)
                  tot+=portArrays[i].merce[j].quantita;

            }
                printf("\n La merce %d è presente in totale su tutti i porti in quantità pari a %d",j,tot);
            }

        }
        //if(semctl(semDaysId,indicePorto,GETVAL)==SO_DAYS)
            //break;
        sleep(0.2);
        if(giorniSimulazione==SO_DAYS-1)
            break;
    }
    printf("\n porto %d muore",indicePorto);
    exit(EXIT_SUCCESS);
}

