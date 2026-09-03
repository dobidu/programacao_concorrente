/* thread_attr.c - pthread_attr_t: pilha, detach e o que o padrão promete.
 *
 * A página de atributos afirma três coisas que este programa torna observáveis:
 *
 *   1. o tamanho de pilha é um ATRIBUTO, e o padrão da glibc é de 8 MB de
 *      espaço virtual por thread - número que decide quantas threads cabem num
 *      processo bem antes de a memória física acabar;
 *   2. uma thread detached não pode ser esperada: o descritor é liberado no
 *      término, e pthread_join sobre ela devolve EINVAL em vez de bloquear;
 *   3. a política de escalonamento é herdada por padrão, e mudá-la exige
 *      PTHREAD_EXPLICIT_SCHED - pedir prioridade sem isso não produz erro
 *      algum, apenas não tem efeito, que é o modo mais silencioso de falhar.
 *
 *     gcc -Wall -Wextra -pthread thread_attr.c -o thread_attr
 *     ./thread_attr
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void *trabalhar(void *arg) {
    (void)arg;
    struct timespec pausa = { 0, 50000000L };
    nanosleep(&pausa, NULL);
    return NULL;
}

int main(void) {
    pthread_attr_t attr;
    size_t tam = 0;

    /* --- 1: o tamanho de pilha padrao --- */
    if (pthread_attr_init(&attr)) { perror("attr_init"); return 1; }
    pthread_attr_getstacksize(&attr, &tam);
    printf("pilha padrao:            %zu bytes (%.1f MB por thread)\n",
           tam, tam / 1048576.0);
    printf("  1000 threads reservam: %.1f GB de espaco virtual\n",
           tam * 1000.0 / 1073741824.0);

    /* Reduzir a pilha e o que permite muitas threads num processo so. O minimo
       portavel e PTHREAD_STACK_MIN, e descer abaixo dele nao e valido. */
    if (pthread_attr_setstacksize(&attr, 256 * 1024)) { perror("setstacksize"); return 1; }
    pthread_attr_getstacksize(&attr, &tam);
    printf("pilha reduzida:          %zu bytes (%.0f KB)\n", tam, tam / 1024.0);

    pthread_t t;
    if (pthread_create(&t, &attr, trabalhar, NULL)) { perror("create"); return 1; }
    pthread_join(t, NULL);
    pthread_attr_destroy(&attr);

    /* --- 2: detached nao pode ser esperada --- */
    if (pthread_attr_init(&attr)) { perror("attr_init"); return 1; }
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&t, &attr, trabalhar, NULL)) { perror("create"); return 1; }

    int r = pthread_join(t, NULL);
    printf("join numa detached:      %s (esperado: EINVAL)\n",
           r == EINVAL ? "EINVAL" : strerror(r));
    pthread_attr_destroy(&attr);

    /* --- 3: pedir politica sem EXPLICIT_SCHED nao produz erro, e nao tem efeito --- */
    if (pthread_attr_init(&attr)) { perror("attr_init"); return 1; }
    int herdado = 0;
    pthread_attr_getinheritsched(&attr, &herdado);
    printf("inheritsched padrao:     %s\n",
           herdado == PTHREAD_INHERIT_SCHED ? "INHERIT_SCHED (a politica pedida e IGNORADA)"
                                            : "EXPLICIT_SCHED");

    struct sched_param p;
    memset(&p, 0, sizeof p);
    p.sched_priority = 10;
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);   /* aceito, e sem efeito */
    pthread_attr_setschedparam(&attr, &p);

    int politica = -1;
    pthread_attr_getschedpolicy(&attr, &politica);
    printf("politica no atributo:    %s, porem herdada na criacao\n",
           politica == SCHED_FIFO ? "SCHED_FIFO" : "outra");
    printf("  para valer:            pthread_attr_setinheritsched(&attr,"
           " PTHREAD_EXPLICIT_SCHED)\n");
    printf("  e ainda assim:         SCHED_FIFO exige privilegio; sem ele,"
           " pthread_create devolve EPERM\n");
    pthread_attr_destroy(&attr);

    /* Nao se cria a thread de tempo real aqui de proposito: no laboratorio ela
       falharia com EPERM, e um exemplo que so roda como root nao e exemplo. */
    return 0;
}
