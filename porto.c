#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <signal.h>
#include <bits/types/sigset_t.h>
#include <bits/sigaction.h>
#include <bits/types/struct_timespec.h>

#include "utility.h"

#define TEST_ERROR  if(errno){ fprintf(stderr,"%s:%d:PID=%5d:Error %d (%s)\n", __FILE__,__LINE__,getpid(),errno,strerror(errno)); }
time_t endwait, actualTime;
int so_navi=0,so_porti=0,so_merci=0,so_min_vita=0,
        so_max_vita=0,so_lato=0,so_speed=0,so_capacity=0,
        so_banchine=0,so_fill=0,so_loadspeed=0,so_days=0,
        so_storm_duration=0,so_swell_duration=0,so_maelstrom=0, so_size=0;
int chiudo=0;
int l,totalePorto,a;
float ricevutaOggi=0;
float speditaOggi=0;
int giorniSimulazione = 0, idSemBanchine, indicePorto;
int banchineOccupate=0;
int merceScaduta=0;

void createIPCKeys(){
    keyPortArray = ftok("master.c", getppid());
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyPortArray");
    }

    keySemPortArray = ftok("nave.c", getppid());
    if(keySemPortArray == -1){
        TEST_ERROR
        perror("errore keySemPortArray");
    }
    keyMessageQueue = ftok("porto.c", getppid());
    if(keyMessageQueue == -1){
        TEST_ERROR
        perror("errore keyMessageQueue");
    }
    keySemMessageQueue = ftok("utility.c", getppid());
    if(messageQueueId == -1){
        TEST_ERROR
        perror("errore keySemMessageQueueId");
    }

    keyReport = ftok("MakeFile", getppid());
    if(keyPortArray == -1){
        TEST_ERROR
        perror("errore keyReport");
    }
}


void comunicazioneNave(int numSemBanchina) {

    int sep = 0, skip = 0,i = 0;
    struct messagebuf buf;
    struct messagebuf buf1;

    char *messaggio = malloc(30 * so_merci);
    char workString[30]; /*string temporanea per poi scrivere tutto il messaggio */
    char delim[] = "|";
    int scadenza=0;
    int quantitaAttuale = 0;
    int ron = 2;/*richiesta offerta non */
    int nomeMerceChiesta = 0;
    char tmp[20];
    char *ptr ;
    int pidAsked = 0;

    if ((messageQueueId = msgget(keyMessageQueue, IPC_CREAT | 0666)) == -1) {
        perror("client: Failed to create message queue:");
        exit(2);
    }
    if ((msgrcv(messageQueueId, &buf, sizeof(buf.mText), getpid(), IPC_NOWAIT)) == -1) {
        if(errno==ENOMSG){
            printf("non è stato trovato il messaggio richiesto, skip, porto.c\n");
            skip = 1;
        }
        else if(errno==EINTR){
            printf("messaggio perso, funzione interrotta da un segnale");
        }
        else if(errno==EINVAL){
            printf("messaggio perso, inserito valore scorretto, messageQueueId: %d\n msgz: %lu, msgtyp(pidProcessoPorto che riceve): %d\n",messageQueueId,sizeof(buf1.mText),getpid());
        }
        else{
            TEST_ERROR
        }
    }
    else{
        printf("messaggio ricevuto, inizio comunicazione, porto.c\n");
    }
    if(skip != 1){
        banchineOccupate+=1;
        report->banchine[indicePorto]+=1;
       /* printf("\n %s",buf.mText);*/
        ptr = strtok(buf.mText, delim);

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
                        printf("\ncompra");
                        if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita >= quantitaAttuale) {
                            portArrays[indicePorto].merce[nomeMerceChiesta].quantita = quantitaAttuale;
                            ricevutaOggi += quantitaAttuale; /*tiene conto delle merci comprate oggi */
                            report->consegnataDaNave[nomeMerceChiesta]+=quantitaAttuale;
                            quantitaAttuale = 0;
                            portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda = 2;
                            ron = 2;
                        } else if (portArrays[indicePorto].merce[nomeMerceChiesta].quantita < quantitaAttuale) {
                            int differenza=quantitaAttuale- portArrays[indicePorto].merce[nomeMerceChiesta].quantita;
                            portArrays[indicePorto].merce[nomeMerceChiesta].quantita=quantitaAttuale-differenza;
                            quantitaAttuale =differenza;
                            portArrays[indicePorto].merce[nomeMerceChiesta].offertaDomanda = 1;
                            ricevutaOggi += portArrays[indicePorto].merce[nomeMerceChiesta].quantita;

                            report->consegnataDaNave[nomeMerceChiesta]+=portArrays[indicePorto].merce[nomeMerceChiesta].quantita;
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


        buf1.mType = pidAsked;

        strcpy(buf1.mText, messaggio);

        if ((msgsnd(messageQueueId, &buf1, sizeof(buf1.mText), 0)) == -1) {
            if(errno==EINTR){
                printf("messaggio perso, funzione interrotta da un segnale");
            }
            else if(errno==EINVAL){
                printf("messaggio perso, inserito valore scorretto, messageQueueId: %d\n msgz: %lu, msgtyp(pidProcessoPorto che invia): %d\n",messageQueueId,sizeof(buf1.mText),getpid());
            }
            else{
                TEST_ERROR
            }
        } else {
            /*decrementare semaforo a 0 */
            if (initSemAvailableTo0(portArrays[indicePorto].semIdBanchinePorto, numSemBanchina) == -1) {
                printf("errore durante l'incremento del semaforo per scrivere sulla coda di messaggi ");
                TEST_ERROR;
            }
            printf("messaggio spedito ala nave, fine comunicazione, porto.c\n");
        }
        /* Porto finisce la trattazione */
        for(i=0;i<so_banchine;i++){
            if(semctl(portArrays[indicePorto].semIdBanchinePorto,i,GETVAL) == 1){
                comunicazioneNave(i);
            }
        }
    }

}

void findScambi(){
    int i;
    for(i=0;i<so_banchine;i++){
        if(semctl(portArrays[indicePorto].semIdBanchinePorto,i,GETVAL) == 1){
            comunicazioneNave(i);
        }
    }
}


/*funzione che riempe l'array di struct dei porti */
void setPorto(){
    int i,j,k,numSem;
    semPortArrayId = semget(keySemPortArray,SO_PORTI,IPC_CREAT | 0666);
    for(i=0;i<SO_PORTI;i++){
        numSem = semctl(semPortArrayId,i,GETVAL);
        if(numSem == -1){
            TEST_ERROR;
        }
        if(numSem == 1){
            if(reserveSem( semPortArrayId, i)==-1){ /*richiede la memoria e la occupa SOLO LUI */
                printf("errore durante il decremento del semaforo per inizializzare il porto");
                perror(strerror(errno));
            }else{
                indicePorto=i;
            }
            break;
        }
    }


    if(portArrays[indicePorto].idPorto==0){
        portArrays[indicePorto].idPorto=getpid();
        portArrays[indicePorto].semIdBanchinePorto = semget(IPC_PRIVATE,so_banchine,IPC_CREAT | 0666);
        for(j=0;j<so_banchine-1;j++){
            initSemAvailableTo0(portArrays[indicePorto].semIdBanchinePorto,j);
        }
        if(indicePorto==0){ /*inizializza porto */
            portArrays[indicePorto].x=0;
            portArrays[indicePorto].y=0;
        }else if(indicePorto==1){
            portArrays[indicePorto].x=so_lato;
            portArrays[indicePorto].y=0;
        }else if(indicePorto==2){
            portArrays[indicePorto].x=so_lato;
            portArrays[indicePorto].y=so_lato;
        }else if(indicePorto==3){
            portArrays[indicePorto].x=0;
            portArrays[indicePorto].y=so_lato;
        }else {
            srand(getpid());
            portArrays[indicePorto].x=(rand() %  (int)so_lato+1);
            portArrays[indicePorto].y=(rand() %  (int)so_lato+1);
            for(j=0;j<indicePorto-1;j++){ /*controllo che non si crei sulla posizione di un altro porto */
                if(portArrays[indicePorto].x== portArrays[j].x && portArrays[indicePorto].y==portArrays[j].y){
                    j=-1;
                    portArrays[indicePorto].x=(rand() %  (int)so_lato+1);
                    portArrays[indicePorto].y=(rand() %  (int)so_lato+1);
                }

            }
        }

    }
    for(k=0;k<so_merci;k++){
        srand((getpid()));
        portArrays[indicePorto].merce[k].nomeMerce = k;
        portArrays[indicePorto].merce[k].offertaDomanda = (rand() %  2);/*0 = domanda, 1 = offerta, 2 = da assegnare */
        if(portArrays[indicePorto].merce[k].offertaDomanda ==1)
            portArrays[indicePorto].merce[k].vitaMerce = (so_min_vita + (rand() %  (so_max_vita-so_min_vita))); /*giorni di vita */

    }

}

void setMerci(){
    int j;
    srand(getpid());
    for(j=0;j<so_merci;j++){

        portArrays[indicePorto].merce[j].nomeMerce = j;
        portArrays[indicePorto].merce[j].offertaDomanda = (rand() %  2);/*0 = domanda, 1 = offerta, 2 = da assegnare */
        if(portArrays[indicePorto].merce[j].offertaDomanda ==1)
            portArrays[indicePorto].merce[j].vitaMerce = (so_min_vita + (rand() %  (so_max_vita-so_min_vita))); /*giorni di vita */
        else
            portArrays[indicePorto].merce[j].vitaMerce =0;

        if(indicePorto==0&&j==0){
            portArrays[indicePorto].merce[j].quantita=((rand() %  ((so_size/2)/so_merci)));

        }else  if ( j==so_merci-1)
            portArrays[indicePorto].merce[j].quantita=rand() % (so_size/2)-sum;
        else {
            portArrays[indicePorto].merce[j].quantita=( rand() % ((so_size/2)-sum)/(so_merci-j) );

        }
        if(portArrays[indicePorto].merce[j].quantita>0)
            sum+=portArrays[indicePorto].merce[j].quantita;

        if(sum>so_size)
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
    int totale,i,j,spawned;
    srand(getpid());


    for(i=0;i<so_merci;i++){
        totale=portArrays[indicePorto].merce[i].quantita;
    }

    j = rand() % (501+so_merci);
    while(j>so_merci)
        j-=so_merci;
    if(totale+(so_size / so_days)<so_size){/*CONTROLLA CHE IL TOTALE NON VENGA MAI SUPERATO */
        if (portArrays[indicePorto].merce[j].offertaDomanda == 1 || portArrays[indicePorto].merce[j].offertaDomanda == 2) {/*0 = domanda, 1 = offerta, 2 = da assegnare */
            if (portArrays[indicePorto].merce[j].offertaDomanda == 2) {
                portArrays[indicePorto].merce[j].vitaMerce = (so_min_vita + (rand() % (so_max_vita - so_min_vita)));  /*giorni di vita */
                spawned=rand() % (1+((so_size/so_merci))/so_days);
                portArrays[indicePorto].merce[j].quantita = spawned;
                report->offerte[indicePorto]+=spawned;
                /*report->ricevuteOggi[indicePorto]+=((so_size / so_days) );
                report->ricevutePorto[indicePorto]+=((so_size / so_days) );*/
                ricevutaOggi +=spawned;
            } else if (portArrays[indicePorto].merce[j].offertaDomanda == 1) {
                portArrays[indicePorto].merce[j].quantita += spawned;
                report->offerte[indicePorto]+=spawned;
                ricevutaOggi +=spawned;

            }


        }}


}
void gestioneInvecchiamentoMerci(){ /*funzione da richiamare ogni "giorno" di simulazione per controllare se la merce del porto è scaduta */

    int k;

    for(k=0;k<so_merci;k++){
        if(portArrays[indicePorto].merce[k].offertaDomanda==1){
            if(portArrays[indicePorto].merce[k].vitaMerce <=0&&portArrays[indicePorto].merce[k].offertaDomanda==1){ /*decidere se cancellare o inizializzare */
                portArrays[indicePorto].merce[k].offertaDomanda=2;
                portArrays[indicePorto].merce[k].vitaMerce=0;
                merceScaduta+= portArrays[indicePorto].merce[k].quantita;
                report->merciScadutePorto[k]+=portArrays[indicePorto].merce[k].quantita;
                portArrays[indicePorto].merce[k].quantita=0;
            }
            else{
                portArrays[indicePorto].merce[k].vitaMerce-=1;
            }
        }
    }
}


void handle_signal(int signum) {
    int i=0;
    struct timespec tim, tim2;
    char str[10];
    tim.tv_sec = 0;
    if(so_swell_duration<10)
        sprintf(str,"%d",so_swell_duration*10);
    else
        sprintf(str,"%d",so_storm_duration);
    strcat(str,"000000L");
    tim.tv_nsec = atoi(str);
    switch (signum) {

        case SIGUSR1:
            actualTime = time(NULL);
            nanosleep(&tim,&tim2);
            endwait = actualTime - time(NULL);
            if(endwait > 1){
                giorniSimulazione += (int) endwait;
            }
            break;
        case SIGUSR2:
            giorniSimulazione++;
            break;
        case SIGTERM:
            for(i=0;i<so_banchine;i++){
                if (initSemAvailableTo0(portArrays[indicePorto].semIdBanchinePorto, i) == -1) {
                    printf("errore durante il set a 0 del semaforo per le banchine del porto: %d\n durante la terminazione", getpid());
                    TEST_ERROR;
                }
            }

            kill(getpid(),SIGTERM);
        default:
            break;
    }
}

int main(int argc, char *argv[]){

    int i,j,tot,size,migliorRichiesta=0;
    struct sigaction sa;
    sigset_t my_mask;


    so_navi=atoi(argv[0]),so_porti=atoi(argv[1]),so_merci=atoi(argv[2]),so_min_vita=atoi(argv[3]),
    so_max_vita=atoi(argv[4]),so_lato=atoi(argv[5]),so_speed=atoi(argv[6]),so_capacity=atoi(argv[7]),
    so_banchine=atoi(argv[8]),so_fill=atoi(argv[9]),so_loadspeed=atoi(argv[10]),so_days=atoi(argv[11]),
    so_storm_duration=atoi(argv[12]),so_swell_duration=atoi(argv[13]),so_maelstrom=atoi(argv[14]), so_size=atoi(argv[15]);


    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_signal;
    sigemptyset(&my_mask); /* do not mask any signal */
    sa.sa_mask = my_mask;
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGUSR1, &sa, 0);
    sigaction(SIGUSR2, &sa, 0);
    sigaction(SIGTERM, &sa, 0);



    createIPCKeys();
    size = (sizeof(portDefinition) + (sizeof(structMerce) * so_merci)) * SO_PORTI;
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

    /*creo la sm per fare il report*/
    reportId = shmget(keyReport,sizeof(report) ,IPC_CREAT | 0666);
    if(portArrayId == -1){
        printf("errore durante la creazione della memoria condivisa report");
        perror(strerror(errno));
    }

    report =shmat(reportId,NULL,0); /*specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo*/
    if (portArrays == (void *) -1){
        printf("errore durante l'attach della memoria condivisa portArray durante l'avvio dell' inizializzazione");
        perror(strerror(errno));
    }

    setPorto();
    setMerci();


    if ((messageQueueId = msgget(keyMessageQueue, IPC_CREAT | 0666)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }

    if(indicePorto==SO_PORTI-1){
        int g,c,temp=0,portoBest; /*temp è usata per fare la somma delle merci richieste nel porto attuale in quel momento*/
        for (g=0;g<SO_PORTI;g++) {
            for(c=0;c<so_merci;c++){
                if(portArrays[g].merce[c].offertaDomanda==0)
                    temp+=portArrays[g].merce[c].quantita;
                else if(portArrays[g].merce[a].offertaDomanda==1){
                    report->offerte[g]+=portArrays[g].merce[c].quantita;


                }

            }
            if(migliorRichiesta<temp){
                migliorRichiesta=temp;
                portoBest=indicePorto;/*mi salvo il porto con la richiesta migliore*/
            }
            temp=0;

        }
        report->migliorRichiesta=portoBest;
    }
    report->affondate=0;
    while(giorniSimulazione<so_days) {
        int random;/*se raggiunge so navi termina la sim perchè non abbiamo più navi in circolo */
        sigaction(SIGUSR1, &sa, NULL);
        sigaction(SIGUSR2, &sa, NULL);

        if (report->affondate<so_navi) {

            banchineOccupate = 0;
            report->banchine[indicePorto]=0;


            /*casualmente sceglie se generare merce o no */
            srand(getpid());
            random = rand() % 2;

            spawnMerci();
            findScambi();



            report->speditePorto[indicePorto]+=speditaOggi;
            report->spediteOggi[indicePorto]=speditaOggi;
            report->ricevuteOggi[indicePorto]=ricevutaOggi;

            report->ricevutePorto[indicePorto]+=ricevutaOggi;
            /*printf("\n Nel porto %d oggi sono state ricevute %f e spedite %f tonnellate al giorno %d\n", indicePorto, ricevutaOggi, speditaOggi,giorniSimulazione);  OPZIONE PER NON USARE 2 ARRAY NELLE STRUCT, PERO' NON ORDINATO*/

            speditaOggi=0;
            ricevutaOggi=0;
            gestioneInvecchiamentoMerci();

            tot = 0;

            /*QUA VENGONO FATTI I REPORT GIORNALIERI*/
            if (indicePorto == so_porti - 1) {/*report di tutte le merci */
                for(j = 0; j < so_merci; j++) {
                    tot = 0;
                    for(i = 0; i < so_porti; i++) {
                        if (portArrays[i].merce[j].offertaDomanda == 1)
                            tot += portArrays[i].merce[j].quantita;

                    }
                    if(giorniSimulazione==0)
                        report->merciGenerate[j]=tot;

                    printf("\n La merce %d è presente in totale su tutti i porti in quantità pari a %d", j, tot);
                }
                printf("\n\n<==============================>\n");
                if(merceScaduta>0){
                    for(i=0;i<so_merci;i++){
                        printf("\nNelle navi, la merce %d è scaduta in quantità pari a %d",i,report->merciScaduteNave[i]);
                    }
                    for(i=0;i<so_merci;i++){
                        printf("\nNei porti,la merce  %d è scaduta in quantità pari a %d",i,report->merciScadutePorto[i]);
                    }

                }else printf("\nNessuna merce scaduta oggi ne nei porti ne nelle nave");
                printf("\n\n<------------------------------>\n");

                for(i=0;i<so_merci;i++){
                    printf("\nNELLE NAVI,la merce %d è presente in quantità pari a %d",i,report->merci[i]);
                }
                printf("\n\n<==============================>\n");
                for(i=0;i<so_merci;i++){
                    report->merci[i]=0;
                }

                printf("\n*********************\nCi sono %d navi in mare senza carico\nCi sono %d navi in mare con carico\nCi sono %d navi a commerciare ai porti\n*********************",report->senzaCarico,report->conCarico,report->inPorto);
                if(giorniSimulazione<so_days-2){
                    report->conCarico=0;
                    report->inPorto=0;
                    report->senzaCarico=0;}

                for(l=0;l<SO_PORTI;l++){
                    totalePorto=0;
                    for(a=0;a<so_merci;a++){
                        if(portArrays[l].merce[a].offertaDomanda==1)
                            totalePorto+=portArrays[l].merce[a].quantita;
                    }
                    printf("\n\n Nel Porto %d sono:\n -presenti %d tonnellate di merce\n -oggi sono state ricevute %d tonnellate di merce\n -oggi state spedite %d tonnellate di merce\n -oggi sono state occupate %d banchine",l,totalePorto,report->ricevuteOggi[l],report->spediteOggi[l],report->banchine[l]);
                }

                printf("\n\n<==============================>\n Il giorno %d sono state:\n Rallentate %d Navi\n Rallentati %d Porti\n Affondate %d Navi\n<==============================>\n\n",giorniSimulazione,report->rallentate,report->rallentati,report->affondate);


            }/*fine daily report*/

            if (giorniSimulazione == so_days - 1)
                break;
        }else {

            if (report->affondate >= so_navi) {
                if (indicePorto == SO_PORTI - 1)
                    printf("\n\n\n\n\nLA SIMULAZIONE TERMINA PER MANCANZA DI NAVI AL GIORNO %d", giorniSimulazione);
                giorniSimulazione = so_days;
                report->conCarico = 0;
                report->inPorto = 0;
                report->senzaCarico = 0;
            }
        }

        sigemptyset (&my_mask);
        sigfillset(&my_mask);
        sigdelset(&my_mask, SIGUSR2);
        sigsuspend (&my_mask);
    }
    for(i=0;i<so_banchine;i++){
        if (initSemAvailableTo0(portArrays[indicePorto].semIdBanchinePorto, i) == -1) {
            printf("errore durante il set a 0 del semaforo per le banchine del porto: %d\n durante la terminazione", getpid());
            TEST_ERROR;
        }
    }
    exit(EXIT_SUCCESS);
}
