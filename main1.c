#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>

#define TRUE 1

#define QTD_FIL 5 // Quantidade de filósofos.
#define COMENDO 1 // Filósofo está comendo.
#define PENSANDO 2 // Filósofo está pensando.
#define OCUPADO 3 // Garfo ocupado.
#define DESOCUPADO 4 // Garfo desocupado.
#define TEMPO 5 // Tempo máximo que ficam comendo e pensando (em s).

int garfos[QTD_FIL] = {DESOCUPADO, DESOCUPADO, DESOCUPADO, DESOCUPADO, DESOCUPADO};
int filosofos[QTD_FIL];

void* jantar(void*);
int come(int*, int*, int*);
int pensa(int*, int*, int*);

int main(){
    srand(time(NULL));

    for(int i = 0; i < QTD_FIL; i++)
        filosofos[i] = i;

    pthread_t tids[QTD_FIL];

    int i;
    for(i = 0; i < QTD_FIL - 1; i++)
        pthread_create(&tids[i], NULL, jantar, &filosofos[i]);
        
    pthread_create(&tids[i], NULL, jantar, &filosofos[i]);

    for(int i = 0; i < QTD_FIL; i++)
        pthread_join(tids[i], NULL);
        
    exit(EXIT_SUCCESS);
}

void* jantar(void* f_numero){
    int f_n = *(int*) f_numero;
    printf("Filosofo %d juntou-se à mesa!\n\n", f_n+1);
    int *filosofo = &filosofos[f_n];
    *filosofo = PENSANDO;
    int *ge = &garfos[f_n], *gd = &garfos[(f_n+1) % QTD_FIL];
    int comeu, pensou;

    while(TRUE){
        // Filosofo começa a comer com os
        // garfos ge (esquerdo) e gd (direito)
        if((comeu = come(filosofo, ge, gd)) == 0)
            printf("Filósofo %d não pôde comer.\n", f_n+1);
        else if(comeu == 1) printf("Filósofo %d comeu.\n\n", f_n+1);

        // Filosofo começa a pensar com os
        // garfos ge (esquerdo) e gd (direito),
        // isto é, devolve-os para a mesa
        pensa(filosofo, ge, gd);
        printf("Filósofo %d pensou.\n\n", f_n+1);
    }
}

/* Dado um filósofo e seus garfos da esquerda e
   da direita, verifica se o filósofo pode começar
   a comer (está pensando), assim, tenta pegar os
   garfos (retorna -1 se já está comendo e 0 se não
   é possível começar a comer - garfo(s) ocupado(s)).
*/
int come(int* filosofo, int* ge, int* gd){
    if(*filosofo == COMENDO) return -1;
 
    int r = 1 + (rand() % TEMPO); // Come por [1,TEMPO] s
    int ge_anterior = *ge; // Estado anterior do garfo à esquerda
    int gd_anterior = *gd; // Estado anterior do garfo à direita

    // Se garfo esquerdo desocupado, pega ele
    // Caso contrário, não come
    if(ge_anterior == DESOCUPADO){
        *ge = OCUPADO;
        printf("Garfo esquerdo foi pego.\n");
    }
    else{
        printf("Garfo esquerdo ocupado.\n");
        return 0;
    }

    // Já pegou o garfo esquerdo, agora pega o
    // garfo direito, se não pegou, não come  
    if(gd_anterior == DESOCUPADO){
        *ge = OCUPADO;
        printf("Garfo direito foi pego.\n");
    }
    else{
        printf("Garfo direito ocupado.\n");
        return 0;
    }

    // Começou a comer aqui
    *filosofo = COMENDO;

    sleep(r);

    return 1;
}

/* Dado um filósofo e seus garfos da esquerda e
   da direita, verifica se o filósofo pode começar
   a pensar (não está pensando ainda), desocupando
   assim os garfos (retorna -1 se já está pensando).
*/
int pensa(int* filosofo, int* ge, int* gd){
    if(*filosofo == PENSANDO) return -1;
 
    // Logo, comendo.
    int r = 1 + (rand() % TEMPO); // Pensa por [1,TEMPO] s

    *ge = DESOCUPADO;
    printf("Garfo esquerdo foi colocado na mesa.\n");

    // Começou a pensar aqui
    *filosofo = PENSANDO;

    *gd = DESOCUPADO;
    printf("Garfo direito foi colocado na mesa.\n");

    sleep(r);

    return 1;
}