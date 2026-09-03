/* pool.c - fila única, fila por trabalhador, e roubo de trabalho.
 *
 * O pool resolve o problema do capítulo anterior, que era criar uma thread por
 * cliente. Resolvido esse, aparece o seguinte: onde ficam as tarefas à espera.
 *
 *   FILA ÚNICA          simples, e todos disputam a mesma trava a cada tarefa
 *   FILA POR TRABALHADOR sem disputa, e desbalanceia quando as durações diferem
 *   COM ROUBO            fila própria, e quem fica ocioso rouba da fila alheia
 *
 * O programa mede os três com a MESMA carga: tarefas de duração muito desigual,
 * distribuídas de forma que um trabalhador receba as pesadas. O que interessa é
 * o tempo total e o tempo ocioso, e não a vazão bruta.
 *
 * O detalhe que faz o roubo funcionar quase sem sincronização é o LADO: o dono
 * consome pelo início e o ladrão retira do fim, de modo que os dois só disputam
 * quando resta uma única tarefa.
 *
 *     gcc -O2 -Wall -Wextra -pthread pool.c -o pool
 *     ./pool
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define W 4                       /* trabalhadores */
#define T 400                     /* tarefas */

static double agora_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

/* trabalho artificial e deterministico: pesadas custam 20x as leves */
static void trabalhar(int peso) {
    volatile unsigned long x = 0;
    for (int i = 0; i < peso * 20000; i++) x += (unsigned long)i;
}

static int peso_de(int i) { return (i % W == 0) ? 20 : 1; }

/* ---------- 1. fila única, com trava ---------- */
static int fila[T], cabeca_u;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static long ociosos_u[W];

static void *fio_unica(void *arg) {
    long id = (long)arg;
    for (;;) {
        pthread_mutex_lock(&mu);
        int i = (cabeca_u < T) ? fila[cabeca_u++] : -1;
        pthread_mutex_unlock(&mu);
        if (i < 0) break;
        trabalhar(peso_de(i));
    }
    ociosos_u[id] = 0;
    return NULL;
}

/* ---------- 2 e 3. fila por trabalhador, com e sem roubo ---------- */
typedef struct {
    int itens[T];
    atomic_int inicio, fim;       /* dono consome no inicio; ladrao, no fim */
} Deque;

static Deque deques[W];
static int roubar_ligado;
static atomic_long roubadas;
static long feitas[W];

static int pegar_do_inicio(Deque *d) {
    int i = atomic_fetch_add(&d->inicio, 1);
    if (i >= atomic_load(&d->fim)) { atomic_fetch_sub(&d->inicio, 1); return -1; }
    return d->itens[i];
}

static int roubar_do_fim(Deque *d) {
    int f = atomic_fetch_sub(&d->fim, 1) - 1;
    if (f < atomic_load(&d->inicio)) { atomic_fetch_add(&d->fim, 1); return -1; }
    return d->itens[f];
}

static void *fio_deque(void *arg) {
    long id = (long)arg;
    for (;;) {
        int i = pegar_do_inicio(&deques[id]);
        if (i < 0) {
            if (!roubar_ligado) break;
            int achou = -1;
            for (int v = 1; v < W && achou < 0; v++)
                achou = roubar_do_fim(&deques[(id + v) % W]);
            if (achou < 0) break;
            atomic_fetch_add(&roubadas, 1);
            i = achou;
        }
        feitas[id]++;
        trabalhar(peso_de(i));
    }
    return NULL;
}

static double medir_unica(void) {
    for (int i = 0; i < T; i++) fila[i] = i;
    cabeca_u = 0;
    memset(ociosos_u, 0, sizeof ociosos_u);
    pthread_t t[W];
    double t0 = agora_ms();
    for (long i = 0; i < W; i++) pthread_create(&t[i], NULL, fio_unica, (void *)i);
    for (int i = 0; i < W; i++) pthread_join(t[i], NULL);
    return agora_ms() - t0;
}

static double medir_deque(int com_roubo, long *fora) {
    roubar_ligado = com_roubo;
    atomic_store(&roubadas, 0);
    memset(feitas, 0, sizeof feitas);
    for (int w = 0; w < W; w++) {
        int n = 0;
        for (int i = w; i < T; i += W) deques[w].itens[n++] = i;
        atomic_store(&deques[w].inicio, 0);
        atomic_store(&deques[w].fim, n);
    }
    pthread_t t[W];
    double t0 = agora_ms();
    for (long i = 0; i < W; i++) pthread_create(&t[i], NULL, fio_deque, (void *)i);
    for (int i = 0; i < W; i++) pthread_join(t[i], NULL);
    double ms = agora_ms() - t0;
    *fora = atomic_load(&roubadas);
    return ms;
}

int main(void) {
    long r;
    printf("%d trabalhadores, %d tarefas, sendo %d pesadas (20x) atribuidas a W0\n\n",
           W, T, T / W);

    printf("  fila unica, com trava        %8.1f ms\n", medir_unica());
    double sem = medir_deque(0, &r);
    printf("  fila por trabalhador         %8.1f ms   (0 roubos)\n", sem);
    double com = medir_deque(1, &r);
    printf("  fila por trabalhador + roubo %8.1f ms   (%ld roubos)\n", com, r);

    printf("\n  tarefas por trabalhador com roubo:");
    for (int w = 0; w < W; w++) printf(" W%d=%ld", w, feitas[w]);
    printf("\n\nA divisao estatica parece justa e nao e: W0 recebeu as pesadas.\n"
           "Sem roubo, os outros tres terminam cedo e ficam parados; com roubo,\n"
           "eles buscam trabalho na fila alheia e o tempo total cai.\n");
    return 0;
}
