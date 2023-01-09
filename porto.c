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
float ricevutaOggi=100;
float speditaOggi=150;
int giorniSimulazione = 0, idSemBanchine, indicePorto;

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
}

void interpretaSezioneMessaggio(const char sezioneMessaggio[], int indiceMerce){
    int check = 1;

    for(int i = 0;i < strlen(sezioneMessaggio) && check != 0;i++){
        int commaCounter = 0;
        char c = sezioneMessaggio[i];
        char value[10] = " ";
        if(c != ';')
            strcat(value,&c);
        else{
            commaCounter++;
            if(commaCounter == 1)
                buf->mType = atoi(value);
            else if(commaCounter == 2 && portArrays[indicePorto].merce[indiceMerce].offertaDomanda == atoi(value)){
                strcpy(value," ");
            }else if(commaCounter == 2 && portArrays[indicePorto].merce[indiceMerce].offertaDomanda != atoi(value)){
                check = 0;
                strcpy(value," ");
            }else if(commaCounter == 3 && portArrays[indicePorto].merce[indiceMerce].nomeMerce != atoi(value)){
                strcpy(value," ");
            }
            else if(commaCounter == 4){
                portArrays[indicePorto].merce[indiceMerce].quantita = portArrays[indicePorto].merce[indiceMerce].quantita - atoi(value);
                strcpy(value," ");
            }

        }
    }

}

void comunicazioneNave(int numSemBanchina){

    char msg[10 * SO_MERCI];
    char workString[20];

    if (msgrcv(messageQueueId, &buf, sizeof(buf->mText), getpid(), IPC_NOWAIT) == -1) {
        perror("msgrcv");
        exit(1);
    }
    else{
        int indiceMerce = 0;
        for(int i = 0;i < strlen(buf->mText);i++){
            int commaCounter = 0;
            char c = buf->mText[i];
            if(c == ';')
                commaCounter++;
            if(commaCounter == 4){
                interpretaSezioneMessaggio(msg, indiceMerce);
                indiceMerce++;
                strcpy(msg," ");
            }
            else
                strcat(msg,&c);
        }
    }

    //for di creazione messaggio per il porto desiderato
    for(int i = 0;i < SO_MERCI;i++){
        sprintf(workString,"%d",portArrays[indicePorto].merce[i].offertaDomanda);
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%c",';');
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%d",portArrays[indicePorto].merce[i].quantita);
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%c",';');
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%d",portArrays[indicePorto].merce[i].vitaMerce);
        strcat(msg,workString);
        strcpy(workString, "");
        sprintf(workString,"%c",';');
        strcat(msg,workString);
        strcpy(workString, "");
        strcpy(buf->mText,msg);
        strcpy(msg, "");
    }

    if((msgsnd(messageQueueId,&buf,sizeof(buf->mText),0))==-1){
        printf("Errore mentre faceva il messaggio");
        perror(strerror(errno));
    }else{
        printf("messaggio spedito");
        //settare semaforo a 2
    }

    if(releaseSem(idSemBanchine,numSemBanchina)==-1){
        printf("errore durante l'incremento del semaforo per scrivere sulla coda di messaggi in nave.c");
        perror(strerror(errno));
    }

}

void findScambi(){
    int numSem;
    for(int i=0;i<SO_BANCHINE;i++){
        numSem = semctl(idSemBanchine,i,GETVAL);
        if(numSem == 2)
            comunicazioneNave(i);
    }
}

//funzione che riempirà le struct dei porti
void setPorto(){


    while(portArrays[indicePorto].idPorto!=0){
        indicePorto++;
    }

    /*for(int i=0;i<SO_PORTI-1;i++){
        initSemAvailable(semPortArrayId,i);
    }*/
    int numSem;
    semPortArrayId = semget(keySemPortArray,SO_PORTI-1,0);
    printf("semPortArrayId: %d \n", semPortArrayId);
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
            srand(time(NULL));
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

void gestioneInvecchiamentoMerci(){ //funzione da richiamare ogni "giorno" di simulazione per checkare se la merce del porto è scaduta
    for(int j=0;j<SO_PORTI;j++){
        for(int k=0;k<SO_MERCI;k++){
            if(portArrays[j].merce[k].vitaMerce <=0){ //decidere se cancellare proprio o settare a 0 e da assegnare il tutto
                portArrays[j].merce[k].offertaDomanda=2;
                portArrays[j].merce[k].vitaMerce=0;
            }
            else{
                portArrays[j].merce[k].vitaMerce-=1;
            }
        }
    }
}

int stampaStatoMemoriaa() {
    printf("prova");
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
    while(portArrays[k].idPorto!=getpid() && k<=SO_PORTI)
        indicePorto++;
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
    setPorto();
    //sleep(k);
    int k=0;
    while(portArrays[k].idPorto!=getpid() && k<=SO_PORTI)
        k++;
    setMerci();

    printf("Mi trovo sul porto n :%d \n",indicePorto);
    //stampaStatoMemoriaa();

    for(int i = 0;i<SO_PORTI;i++) {

        printf("X del porto %d: %d   \n", i, portArrays[i].x);
        printf("Y del porto %d: %d   \n", i, portArrays[i].y);
        printf("ID DEL PORTO :%d \n", portArrays[i].idPorto);

    }
     /*   printf("X del porto %d: %d   \n",k,portArrays[k].x);
        printf("Y del porto %d: %d   \n",k,portArrays[k].y);
        printf("ID DEL PORTO :%d \n",portArrays[k].idPorto);
*/
    for(int j=0;j<SO_MERCI;j++){
        int q=portArrays[k].merce[j].quantita;
        
       printf("\n PORTO NUMERO:%d La merce numero %d e' richieta/offerta/non (%d)  in qualita' pari a :%d tonnellate con una vita (se venduta)  di %d giorni \n",k,portArrays[k].merce[j].nomeMerce,portArrays[k].merce[j].offertaDomanda,q,portArrays[k].merce[j].vitaMerce);//,portArrays[k].merce[j].quantita

    }
   // reportGiornalieroPorto();
   //
   //}


    if ((messageQueueId = msgget(keyMessageQueue, 0)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }
    printf("message queue: ready to receive messages.\n");



    while(SO_DAYS-giorniSimulazione>0){

        findScambi();
        /*reportGiornalieroPorto();*/
        printf("Giorno: %d.\n",giorniSimulazione);
        giorniSimulazione++;
        sleep(2);
    }

    exit(EXIT_SUCCESS);
}


