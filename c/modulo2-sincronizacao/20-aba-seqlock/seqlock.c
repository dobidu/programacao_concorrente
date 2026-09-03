/* seqlock.c - o contador de sequência, e o ABA que ele resolve.
 *
 * O CAS compara valores, e não histórias: se o valor voltar a ser o que era
 * entre a leitura e a troca, o CAS tem sucesso sobre um estado que já não é o
 * observado. A correção padrão é versionar - comparar o par valor mais
 * contador, incrementado a cada modificação. O ponteiro pode voltar; a versão,
 * não.
 *
 * A mesma ideia, aplicada do lado da LEITURA, produz o seqlock: um contador de
 * sequência é incrementado antes e depois de cada escrita, de modo que valores
 * ímpares indicam escrita em curso. O leitor lê o contador, lê os dados de
 * forma otimista, e relê o contador; se mudou, ou era ímpar, refaz.
 *
 * Este programa mede: leitores nunca bloqueiam e nunca bloqueiam o escritor, e
 * a leitura pode ser refeita indefinidamente sob escrita frequente. As duas
 * afirmações aparecem nos contadores.
 *
 *     gcc -O2 -Wall -Wextra -pthread seqlock.c -o seqlock
 *     ./seqlock
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { long a, b; } Par;       /* invariante: b == a * 2 */

static atomic_ulong seq;                 /* impar = escrita em curso */
static Par dados;
static atomic_int parar;
static long refeitas, lidas, inconsistentes;

static void *escritor(void *arg) {
    long n = *(long *)arg;
    for (long i = 1; i <= n; i++) {
        atomic_fetch_add_explicit(&seq, 1, memory_order_relaxed);   /* impar */
        atomic_thread_fence(memory_order_release);
        dados.a = i;
        dados.b = i * 2;
        atomic_thread_fence(memory_order_release);
        atomic_fetch_add_explicit(&seq, 1, memory_order_relaxed);   /* par */
    }
    atomic_store(&parar, 1);
    return NULL;
}

static void *leitor(void *a) {
    (void)a;
    while (!atomic_load(&parar)) {
        unsigned long v0, v1;
        Par copia;
        do {
            v0 = atomic_load_explicit(&seq, memory_order_acquire);
            if (v0 & 1) { refeitas++; continue; }     /* escrita em curso */
            copia = dados;                            /* leitura otimista */
            atomic_thread_fence(memory_order_acquire);
            v1 = atomic_load_explicit(&seq, memory_order_relaxed);
            if (v0 != v1) refeitas++;
        } while ((v0 & 1) || v0 != v1);

        lidas++;
        if (copia.b != copia.a * 2) inconsistentes++;  /* nunca deve ocorrer */
    }
    return NULL;
}

int main(int argc, char **argv) {
    long n = argc > 1 ? atol(argv[1]) : 2000000;
    pthread_t e, l;

    pthread_create(&l, NULL, leitor, NULL);
    pthread_create(&e, NULL, escritor, &n);
    pthread_join(e, NULL);
    pthread_join(l, NULL);

    printf("escritas:        %ld\n", n);
    printf("leituras validas:%ld\n", lidas);
    printf("leituras refeitas:%ld  (%.1f%% do total de tentativas)\n",
           refeitas, 100.0 * refeitas / (refeitas + lidas));
    printf("inconsistentes:  %ld  <- tem de ser ZERO\n", inconsistentes);
    printf("\nO leitor nunca bloqueou e nunca bloqueou o escritor. O preco esta\n"
           "na coluna das refeitas, e ele cresce com a frequencia de escrita.\n");
    return inconsistentes ? 1 : 0;
}
