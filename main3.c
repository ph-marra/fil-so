#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>

#define TRUE 1

#define QTD_FIL 5 // Quantidade de filósofos.
#define COMENDO 1 // Filósofo está comendo.
#define PENSANDO 2 // Filósofo está pensando.
#define FOME 3 // Filósofo está com fome.
#define OCUPADO 3 // Garfo ocupado.
#define DESOCUPADO 4 // Garfo desocupado.
#define TEMPO 5 // Tempo máximo que ficam comendo e pensando (em s).
#define FILOSOFOS_NUMBER 5568

int garfos[QTD_FIL];

struct sem_filosofo{
    sem_t sem;
    int estado;
};
typedef struct sem_filosofo* Sem_Filosofo;

int shmid_filosofos;
Sem_Filosofo filosofos;

sem_t mutex; // Semáforo para exclusão mútua

void* jantar(void*);
void come(int);
void pensa(int);
void pega_garfos(int);
void larga_garfos(int);

int main(){
    srand(time(NULL));

    key_t filosofos_number = FILOSOFOS_NUMBER;
    shmid_filosofos = shmget(filosofos_number,  QTD_FIL * sizeof(struct sem_filosofo), 0666 | IPC_CREAT);
    if(shmid_filosofos == -1) exit(EXIT_FAILURE);

    void* shm_filosofos = shmat(shmid_filosofos, (void*) 0, 0);
    if(shm_filosofos == (void*) -1) exit(EXIT_FAILURE);

    filosofos = (Sem_Filosofo) shm_filosofos;

    for(int i = 0; i < QTD_FIL; i++)
        sem_init(&filosofos[i].sem, 1, 1);

    for(int i = 0; i < QTD_FIL; i++)
        filosofos[i].estado = PENSANDO;

// -------------------------------------------------------------------- 

    sem_init(&mutex, 1, 1);

    for(int i = 0; i < QTD_FIL; i++) garfos[i] = DESOCUPADO;

// -------------------------------------------------------------------- 

    pthread_t tids[QTD_FIL];

    int is[QTD_FIL];

    // Cria um vetor de ids dos filósofos para diferenciar
    // cada filósofo na função jantar
    for(int i = 0; i < QTD_FIL; i++) is[i] = i;

    int i;
    for(i = 0; i < QTD_FIL - 1; i++)
        pthread_create(&tids[i], NULL, jantar, &is[i]);
        
    pthread_create(&tids[i], NULL, jantar, &is[i]);

    for(int i = 0; i < QTD_FIL; i++)
        pthread_join(tids[i], NULL);
        
    exit(EXIT_SUCCESS);
}

void* jantar(void* f_numero){
    int f_n = *(int*) f_numero;
    printf("Filosofo %d juntou-se à mesa!\n\n", f_n+1);

    while(TRUE){
        pensa(f_n);
        pega_garfos(f_n);
        come(f_n);
        larga_garfos(f_n);
    }
}

/* Dado um id de filósofo (f_n), essa função seta todas as
   informações importantes em relação a esse filósofo, ie,
   ele próprio (estado+semáforo da struct sem_filosofo),
   o filósofo à esquerda, o à direita e os garfos da sua
   mão esquerda e da mão direita.
*/
void seta_infos(int f_n, struct sem_filosofo** filosofo, struct sem_filosofo** fe, struct sem_filosofo** fd, int** ge, int** gd){
    *filosofo = &filosofos[f_n];
    *ge = &garfos[f_n], *gd = &garfos[(f_n+1) % QTD_FIL];
    *fe = NULL, *fd = NULL;

    if(f_n == 0) *fe = &filosofos[QTD_FIL-1];
    if(f_n == QTD_FIL-1) *fd = &filosofos[0];
    if(*fe == NULL) *fe = &filosofos[f_n-1];
    if(*fd == NULL) *fd = &filosofos[f_n+1];
}

/* Dado um id de filósofo (f_n), essa função seta se o
   filósofo pode começar a comer (pegar os garfos), ou
   seja, estado de COMENDO e ocupar os garfos.
*/
void teste(int f_n){
    struct sem_filosofo *filosofo, *fe, *fd;
    int *ge, *gd;
    seta_infos(f_n, &filosofo, &fe, &fd, &ge, &gd);

    if(filosofo->estado == FOME && fe->estado != COMENDO && fd->estado != COMENDO){
        filosofo->estado = COMENDO;
        *ge = OCUPADO;
        *gd = OCUPADO;
        sem_post(&filosofo->sem);
    }
}

/* Dado um id de filósofo (f_n), essa função faz o
   filósofo comer (ou seja, em estado de FOME), testa
   se ele pode pegar os garfos (por teste(f_n)).   
*/
void pega_garfos(int f_n){
    sem_wait(&mutex);

    struct sem_filosofo *filosofo, *fe, *fd;
    int *ge, *gd;
    seta_infos(f_n, &filosofo, &fe, &fd, &ge, &gd);

    filosofo->estado = FOME;
    teste(f_n);
    sem_post(&mutex);
    sem_wait(&filosofo->sem);
}

/* Cálcula o id do filósofo à esquerda */
int f_esq(int f_n){
    if(f_n == 0) return QTD_FIL-1;
    else return (f_n - 1);
}

/* Cálcula o id do filósofo à direita */
int f_dir(int f_n){
    if(f_n == QTD_FIL-1) return 0;
    else return (f_n + 1);
}

/* Dado um id de filósofo (f_n), essa função faz o
   filósofo parar de comer (fica no estado PENSANDO)
   e os garfos são desocupados.
*/
void larga_garfos(int f_n){
    sem_wait(&mutex);

    struct sem_filosofo *filosofo, *fe, *fd;
    int *ge, *gd;
    seta_infos(f_n, &filosofo, &fe, &fd, &ge, &gd);

    filosofo->estado = PENSANDO;
    *ge = DESOCUPADO;
    *gd = DESOCUPADO;
    teste(f_esq(f_n));
    teste(f_dir(f_n));

    sem_post(&mutex);
}

void come(int f_n){
    int r = 1 + (rand() % TEMPO); // Come por [1,TEMPO] s

    printf("Filosofo %d comeu.\n\n", f_n+1);

    sleep(r);  
}

void pensa(int f_n){
    int r = 1 + (rand() % TEMPO); // Come por [1,TEMPO] s

    printf("Filosofo %d pensou.\n\n", f_n+1);

    sleep(r);
}