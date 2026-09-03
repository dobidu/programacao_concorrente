/* ticket.c - justiça do ticket lock, e o regime em que ela desaba.
 *
 * Segurança e vivacidade são propriedades diferentes: a exclusão mútua de um
 * pthread_mutex_t nunca é violada, e ainda assim uma thread pode ser servida
 * muito menos que as outras, porque o padrão POSIX não promete ordem no
 * despertar. O ticket lock promete: cada thread tira um bilhete e é servida em
 * ordem de chegada, com espera limitada a N-1 passagens.
 *
 * O programa mede as duas coisas em dois regimes, e o segundo resultado é o que
 * justifica o exemplo existir.
 *
 * COM MENOS THREADS QUE NÚCLEOS, o mutex do glibc sai quase tão uniforme quanto
 * o ticket - e isso é observação, não garantia: depende da carga, do número de
 * threads e da versão da biblioteca.
 *
 * COM MAIS THREADS QUE NÚCLEOS, o ticket lock desaba. Ele é um spinlock, e a
 * ordem que promete é a ordem dos bilhetes, não a das threads que estão de fato
 * na CPU: quando o dono do bilhete da vez é preemptado, todos os outros giram
 * sem poder avançar. Nesta máquina, com 64 threads em 24 núcleos, a vazão cai
 * cerca de duzentas vezes e a razão entre a thread mais e a menos servida passa
 * de setecentos - o mecanismo JUSTO produziu a distribuição mais injusta das
 * duas, porque justiça de bilhete não é justiça de tempo de CPU.
 *
 * A lição é a do Capítulo 5: a escolha de mecanismo depende do regime, e o
 * regime se descobre medindo no ponto de operação real.
 *
 *     gcc -O2 -Wall -Wextra -pthread ticket.c -o ticket
 *     ./ticket            # 300 ms por medicao
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX 128

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static atomic_ulong proximo, atendendo;
static atomic_int parar;
static long vezes[MAX];
static int usar_ticket;

static void entrar(void) {
    if (usar_ticket) {
        unsigned long meu = atomic_fetch_add_explicit(&proximo, 1,
                                                      memory_order_relaxed);
        while (atomic_load_explicit(&atendendo, memory_order_acquire) != meu)
            ;
    } else {
        pthread_mutex_lock(&m);
    }
}

static void sair(void) {
    if (usar_ticket)
        atomic_fetch_add_explicit(&atendendo, 1, memory_order_release);
    else
        pthread_mutex_unlock(&m);
}

static void *fio(void *arg) {
    long id = (long)arg;
    while (!atomic_load(&parar)) {
        entrar();
        vezes[id]++;
        for (volatile int k = 0; k < 200; k++) ;   /* secao critica curta */
        sair();
    }
    return NULL;
}

static void medir(const char *rotulo, int ticket, int n, long ms) {
    usar_ticket = ticket;
    atomic_store(&parar, 0);
    atomic_store(&proximo, 0);
    atomic_store(&atendendo, 0);
    for (int i = 0; i < n; i++) vezes[i] = 0;

    pthread_t t[MAX];
    for (long i = 0; i < n; i++) pthread_create(&t[i], NULL, fio, (void *)i);

    struct timespec pausa = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&pausa, NULL);
    atomic_store(&parar, 1);
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);

    long total = 0, menor = vezes[0], maior = vezes[0];
    for (int i = 0; i < n; i++) {
        total += vezes[i];
        if (vezes[i] < menor) menor = vezes[i];
        if (vezes[i] > maior) maior = vezes[i];
    }
    printf("  %-8s %-8s total %9ld   menor %8ld   maior %8ld   razao %8.2fx\n",
           rotulo, ticket ? "ticket" : "mutex", total, menor, maior,
           menor ? (double)maior / menor : 0.0);
}

int main(int argc, char **argv) {
    long ms = argc > 1 ? atol(argv[1]) : 300;
    long nucleos = sysconf(_SC_NPROCESSORS_ONLN);
    int poucas = 4;
    int muitas = (int)(nucleos * 2 + 16);
    if (muitas > MAX) muitas = MAX;

    printf("%ld nucleos online, %ld ms por medicao\n\n", nucleos, ms);

    printf("REGIME 1 - %d threads, bem abaixo dos nucleos:\n", poucas);
    medir("poucas", 0, poucas, ms);
    medir("poucas", 1, poucas, ms);

    printf("\nREGIME 2 - %d threads, bem acima dos nucleos:\n", muitas);
    medir("muitas", 0, muitas, ms);
    medir("muitas", 1, muitas, ms);

    printf("\nNo regime 1 os dois sao quase uniformes: o glibc foi justo AQUI,\n"
           "com ESTA carga, o que e observacao e nao garantia.\n"
           "No regime 2 o ticket desaba - ele e um spinlock, e quando o dono do\n"
           "bilhete da vez e preemptado, todos os outros giram sem avancar. A\n"
           "justica de bilhete nao e justica de tempo de CPU.\n");
    return 0;
}
