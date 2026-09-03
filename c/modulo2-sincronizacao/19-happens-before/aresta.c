/* aresta.c - a aresta happens-before, e o que falta sem ela.
 *
 * Há dois tipos de aresta e só dois. Dentro de uma thread, a ordem do programa
 * dá a aresta de graça. Entre threads, ela só existe se um par de operações de
 * sincronização a criar: unlock e lock do mesmo mutex, store release e load
 * acquire da mesma atômica, create e o início da thread, o fim da thread e o
 * join.
 *
 * Este programa demonstra três desses pares, e o assert é a verificação: se a
 * aresta existir, o valor é visível; se não existisse, a leitura seria uma
 * corrida de dados, que o padrão C classifica como comportamento indefinido -
 * e não como "ler um valor antigo".
 *
 *     gcc -O2 -Wall -Wextra -pthread aresta.c -o aresta
 *     ./aresta
 */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

static int dado;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static atomic_int bandeira;

/* 1. create -> inicio da thread: tudo escrito antes do create e visivel */
static void *pelo_create(void *arg) {
    assert(*(int *)arg == 7);
    return NULL;
}

/* 2. unlock -> lock do MESMO mutex */
static void *pelo_mutex(void *a) {
    (void)a;
    pthread_mutex_lock(&m);
    assert(dado == 42);          /* publicado pelo unlock da main */
    dado = 43;
    pthread_mutex_unlock(&m);
    return NULL;
}

/* 3. store release -> load acquire da MESMA atomica */
static void *pelo_atomico(void *a) {
    (void)a;
    while (!atomic_load_explicit(&bandeira, memory_order_acquire)) ;
    assert(dado == 100);         /* publicado junto com a bandeira */
    return NULL;
}

int main(void) {
    pthread_t t;

    /* --- aresta 1: create --- */
    int local = 7;
    pthread_create(&t, NULL, pelo_create, &local);
    pthread_join(t, NULL);       /* e a aresta 4: fim da thread -> join */
    printf("create -> inicio da thread:   aresta existe\n");

    /* --- aresta 2: unlock -> lock --- */
    pthread_mutex_lock(&m);
    dado = 42;
    pthread_mutex_unlock(&m);    /* publica */
    pthread_create(&t, NULL, pelo_mutex, NULL);
    pthread_join(t, NULL);
    assert(dado == 43);          /* o unlock DELA publicou para o join daqui */
    printf("unlock -> lock do mesmo mutex: aresta existe\n");

    /* --- aresta 3: release -> acquire --- */
    dado = 100;
    pthread_create(&t, NULL, pelo_atomico, NULL);
    atomic_store_explicit(&bandeira, 1, memory_order_release);
    pthread_join(t, NULL);
    printf("release -> acquire da atomica: aresta existe\n");

    printf("\nSem qualquer um desses pares NAO ha aresta, e a leitura passa a\n"
           "ser corrida de dados: comportamento indefinido, e nao valor antigo.\n"
           "Um mutex usado so de um lado nao cria aresta nenhuma.\n");
    return 0;
}
