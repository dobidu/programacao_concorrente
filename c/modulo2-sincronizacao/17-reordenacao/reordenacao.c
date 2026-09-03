/* reordenacao.c - r1=0 e r2=0, o resultado que o código parece proibir.
 *
 *     T0:  x = 1;  r1 = y;        T1:  y = 1;  r2 = x;
 *
 * Lendo o programa, r1=0 e r2=0 parece impossível: se r1 leu y antes de T1
 * escrever, T1 executou depois, e portanto leria o x=1 de T0. O raciocínio está
 * correto sob uma hipótese que nenhum processador atual cumpre - a de existir
 * uma ordem global única em que as quatro instruções acontecem.
 *
 * O que existe é o store buffer: a escrita vai para uma fila local do núcleo, e
 * a leitura seguinte pode passar à frente dela. Este programa repete o
 * experimento até o par (0,0) aparecer, e ele aparece - em x86-64, com uma
 * frequência que costuma surpreender quem nunca mediu.
 *
 *     gcc -O2 -Wall -Wextra -pthread reordenacao.c -o reordenacao
 *     ./reordenacao            # ate encontrar, ou 1 milhao de rodadas
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

/* relaxed: atomicidade sem ordenacao. Sao atomicas para que o programa nao
   tenha corrida de dados, que seria comportamento indefinido; a AUSENCIA de
   ordenacao e justamente o que se quer observar. */
static atomic_int x, y;
static int r1, r2;
static pthread_barrier_t partida, chegada;

static void *fio0(void *a) {
    (void)a;
    for (;;) {
        pthread_barrier_wait(&partida);
        atomic_store_explicit(&x, 1, memory_order_relaxed);
        r1 = atomic_load_explicit(&y, memory_order_relaxed);
        pthread_barrier_wait(&chegada);
    }
    return NULL;
}

static void *fio1(void *a) {
    (void)a;
    for (;;) {
        pthread_barrier_wait(&partida);
        atomic_store_explicit(&y, 1, memory_order_relaxed);
        r2 = atomic_load_explicit(&x, memory_order_relaxed);
        pthread_barrier_wait(&chegada);
    }
    return NULL;
}

int main(int argc, char **argv) {
    long limite = argc > 1 ? atol(argv[1]) : 1000000;
    pthread_t a, b;

    pthread_barrier_init(&partida, NULL, 3);
    pthread_barrier_init(&chegada, NULL, 3);
    pthread_create(&a, NULL, fio0, NULL);
    pthread_create(&b, NULL, fio1, NULL);

    long conta[4] = {0};                 /* (0,0) (0,1) (1,0) (1,1) */
    long i;
    for (i = 1; i <= limite; i++) {
        atomic_store(&x, 0);
        atomic_store(&y, 0);
        r1 = r2 = -1;

        pthread_barrier_wait(&partida);
        pthread_barrier_wait(&chegada);

        conta[(r1 ? 2 : 0) + (r2 ? 1 : 0)]++;
        if (r1 == 0 && r2 == 0) break;
    }

    printf("rodadas ate (r1=0, r2=0): %ld\n", i > limite ? -1 : i);
    printf("  (0,0) %ld   <- a ordem global unica nao explica esta\n", conta[0]);
    printf("  (0,1) %ld\n", conta[1]);
    printf("  (1,0) %ld\n", conta[2]);
    printf("  (1,1) %ld\n", conta[3]);
    if (i > limite)
        printf("\nnao apareceu em %ld rodadas: repita, ou aumente o limite.\n", limite);
    else
        printf("\nO par (0,0) aconteceu. Nenhuma ordem das quatro instrucoes o produz.\n");

    /* as threads ficam no laco; o processo termina e o SO recolhe */
    return 0;
}
