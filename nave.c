
#include "Utility.c"
/* Processo nave */







typedef struct { //struct del porto
    float x;
    float y;
    pid_t idNave;
    structMerce *merce;
} naveDefinition;

typedef struct { //CHE CAZZO METTO QUA ANDRE? ADADADASDAD
    naveDefinition *navi;
}ArrayNavi;


static ArrayNavi *naviArray;

void createNaviArray(){ //inizializzo la shared memory

    int i,j;
    char ch = '/';
    char *emptyChar = &ch;

    shm_id = shmget(IPC_PRIVATE,SO_NAVI* sizeof(naveDefinition) + sizeof(size_t),0666);// crea la shared memory con shmget
    naviArray = shmat(shm_id,NULL,0); //specifica l'uso della mem condivista con la system call shmat, che attacca un'area di mem identificata da shmid a uno spazio di processo


    for(i=0;i<SO_NAVI;i++){ //inizializzazione dei campi della struct porti
        naviArray[i].navi = malloc(sizeof(naveDefinition)); //utilizzata la malloc per instanziare il port array
        srand(time(NULL));
        naviArray[i].navi->x=(rand() %  (int)SO_LATO);
        naviArray[i].navi->y=(rand() %  (int)SO_LATO);
        naviArray[i].navi->idNave=0;
        naviArray[i].navi[j].merce = malloc(sizeof(structMerce)); //nasce senza merce

    }

}


