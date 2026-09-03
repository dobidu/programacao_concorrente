/* contencao.c - vazão de quatro mecanismos sob contenção crescente.
 *
 * A página de benchmark do material afirma que o custo relativo entre mutex,
 * spinlock, atômica e semáforo depende da contenção e do tamanho da seção
 * crítica. A afirmação só vale se os números vierem de medição, e é este
 * programa que os produz.
 *
 * O que se mede: N threads incrementando um contador compartilhado o maior
 * número de vezes possível durante um tempo fixo. A vazão é o total de
 * incrementos por segundo, e a comparação entre os mecanismos com o MESMO N é o
 * que interessa - não o valor absoluto, que muda com a máquina.
 *
 * Duas decisões que evitam medir a coisa errada, ambas herdadas do LAB-04:
 *
 *   · as threads são criadas UMA vez, fora do trecho cronometrado. Criá-las por
 *     rodada faria pthread_create dominar o tempo;
 *   · o contador é `volatile` no caso não atômico, senão o compilador o mantém
 *     em registrador e a seção crítica deixa de tocar a memória.
 *
 * O spinlock usa __atomic_test_and_set do próprio gcc, e não pthread_spinlock_t,
 * porque este último não existe em toda plataforma POSIX e o material precisa
 * compilar no laboratório sem condicional.
 *
 *     gcc -O2 -Wall -Wextra -pthread contencao.c -o contencao
 *     ./contencao                 # 1, 2, 4 e 8 threads
 *     ./contencao 200             # com 200 ms por medição
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double agora_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

/* ---- os quatro mecanismos ------------------------------------------ */

static volatile unsigned long g_contador;
static atomic_ulong g_atomico;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t g_sem;
static volatile int g_spin;          /* 0 livre, 1 tomado */

static volatile int g_rodando;
static double g_ate;

typedef struct { int qual; unsigned long feitos; } Tarefa;

static void *trabalhar(void *arg) {
    Tarefa *t = arg;
    unsigned long n = 0;
    while (g_rodando) {
        switch (t->qual) {
        case 0:                                   /* mutex */
            pthread_mutex_lock(&g_mutex);
            g_contador += 1;
            pthread_mutex_unlock(&g_mutex);
            break;
        case 1:                                   /* spinlock */
            while (__atomic_test_and_set(&g_spin, __ATOMIC_ACQUIRE))
                ;
            g_contador += 1;
            __atomic_clear(&g_spin, __ATOMIC_RELEASE);
            break;
        case 2:                                   /* atômica */
            atomic_fetch_add_explicit(&g_atomico, 1, memory_order_relaxed);
            break;
        default:                                  /* semáforo */
            sem_wait(&g_sem);
            g_contador += 1;
            sem_post(&g_sem);
            break;
        }
        n += 1;
        if ((n & 0xFF) == 0 && agora_ms() >= g_ate) break;
    }
    t->feitos = n;
    return NULL;
}

static double medir(int qual, int threads, int ms) {
    pthread_t *fios = calloc((size_t)threads, sizeof *fios);
    Tarefa *tarefas = calloc((size_t)threads, sizeof *tarefas);
    if (!fios || !tarefas) { perror("calloc"); exit(1); }

    g_contador = 0;
    atomic_store(&g_atomico, 0);
    g_spin = 0;
    g_rodando = 1;

    double t0 = agora_ms();
    g_ate = t0 + ms;
    for (int i = 0; i < threads; i++) {
        tarefas[i].qual = qual;
        if (pthread_create(&fios[i], NULL, trabalhar, &tarefas[i])) {
            perror("pthread_create"); exit(1);
        }
    }
    /* a parada é pelo relógio de cada thread; g_rodando é a rede de segurança */
    unsigned long total = 0;
    for (int i = 0; i < threads; i++) {
        pthread_join(fios[i], NULL);
        total += tarefas[i].feitos;
    }
    double gasto = agora_ms() - t0;

    free(fios); free(tarefas);
    return total / gasto / 1000.0;        /* milhões de operações por segundo */
}

int main(int argc, char **argv) {
    int ms = argc > 1 ? atoi(argv[1]) : 150;
    const char *nome[] = { "mutex", "spinlock", "atomica", "semaforo" };
    int ns[] = { 1, 2, 4, 8 };

    if (sem_init(&g_sem, 0, 1)) { perror("sem_init"); return 1; }

    printf("vazão em milhões de operações por segundo, %d ms por medição\n\n", ms);
    printf("%-10s", "threads");
    for (size_t j = 0; j < sizeof nome / sizeof *nome; j++) printf("%12s", nome[j]);
    printf("\n");

    for (size_t i = 0; i < sizeof ns / sizeof *ns; i++) {
        printf("%-10d", ns[i]);
        for (size_t j = 0; j < sizeof nome / sizeof *nome; j++)
            printf("%12.2f", medir((int)j, ns[i], ms));
        printf("\n");
    }
    sem_destroy(&g_sem);
    return 0;
}
