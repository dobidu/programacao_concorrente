/* publicacao.c - o padrão de publicação, com relaxed e com release/acquire.
 *
 *     P:  dado = 42;              C:  while (!pronto) ;
 *         pronto = 1;                 usa(dado);
 *
 * A correção depende inteiramente de a escrita do dado ficar visível ANTES da
 * escrita da bandeira, e nada no código diz isso. `relaxed` garante que a
 * operação sobre `pronto` seja atômica - ninguém lê meia bandeira - e não
 * garante ordenação alguma em relação ao resto.
 *
 * O programa roda os dois modos e conta quantas vezes o consumidor viu a
 * bandeira levantada com o dado ainda em zero. Em x86-64 o hardware já ordena
 * quase tudo e o modo relaxed costuma passar; é por isso que código testado só
 * em x86 quebra ao mudar de arquitetura, e é por isso que o modelo é da
 * LINGUAGEM e não da máquina - o compilador também reordena.
 *
 *     gcc -O2 -Wall -Wextra -pthread publicacao.c -o publicacao
 *     ./publicacao
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

static int dado;                 /* comum: quem ordena e a bandeira */
static atomic_int pronto;
static int usar_release;
static long furos;
static pthread_barrier_t partida, chegada;

static void *produtor(void *a) {
    (void)a;
    for (;;) {
        pthread_barrier_wait(&partida);
        dado = 42;
        if (usar_release) atomic_store_explicit(&pronto, 1, memory_order_release);
        else              atomic_store_explicit(&pronto, 1, memory_order_relaxed);
        pthread_barrier_wait(&chegada);
    }
    return NULL;
}

static void *consumidor(void *a) {
    (void)a;
    for (;;) {
        pthread_barrier_wait(&partida);
        int visto;
        if (usar_release) {
            while (!atomic_load_explicit(&pronto, memory_order_acquire)) ;
        } else {
            while (!atomic_load_explicit(&pronto, memory_order_relaxed)) ;
        }
        visto = dado;
        if (visto != 42) furos++;
        pthread_barrier_wait(&chegada);
    }
    return NULL;
}

static long medir(int release, long rodadas) {
    usar_release = release;
    furos = 0;
    for (long i = 0; i < rodadas; i++) {
        dado = 0;
        atomic_store(&pronto, 0);
        pthread_barrier_wait(&partida);
        pthread_barrier_wait(&chegada);
    }
    return furos;
}

int main(int argc, char **argv) {
    long rodadas = argc > 1 ? atol(argv[1]) : 200000;
    pthread_t p, c;

    pthread_barrier_init(&partida, NULL, 3);
    pthread_barrier_init(&chegada, NULL, 3);
    pthread_create(&p, NULL, produtor, NULL);
    pthread_create(&c, NULL, consumidor, NULL);

    printf("%ld rodadas de cada modo\n\n", rodadas);
    printf("  relaxed:          %ld leitura(s) do dado ainda em zero\n", medir(0, rodadas));
    printf("  release/acquire:  %ld leitura(s) do dado ainda em zero\n", medir(1, rodadas));
    printf("\nEm x86-64 o hardware ja ordena as escritas entre si, entao os dois\n"
           "costumam dar zero. O que relaxed NAO impede e a reordenacao pelo\n"
           "COMPILADOR, e a garantia do padrao vale para toda arquitetura - em\n"
           "ARM e RISC-V o par emite barreira, e o relaxed falha.\n");
    return 0;
}
