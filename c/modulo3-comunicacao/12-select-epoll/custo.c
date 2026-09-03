/* custo.c - select e epoll sob o regime real: muitas conexões, poucas ativas.
 *
 * A diferença entre os dois é assintótica, e o experimento que a torna visível
 * mantém o número de descritores ATIVOS fixo enquanto o total cresce - que é o
 * regime de um servidor: dez mil conexões abertas e algumas dezenas com dados a
 * qualquer instante.
 *
 * Com select, o programa entrega o conjunto inteiro ao núcleo a cada chamada, e
 * o núcleo varre tudo. Com epoll, o interesse é registrado uma vez e cada
 * espera devolve apenas os prontos. O tempo por chamada mostra a diferença, e
 * ela cresce com o total.
 *
 * Os descritores são pipes, e não sockets, porque o efeito é do MECANISMO de
 * espera e não do protocolo - e pipes dispensam rede, porta e privilégio.
 *
 *     gcc -O2 -Wall -Wextra -pthread custo.c -o custo
 *     ./custo
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#define ATIVOS 3
#define RODADAS 300

static double agora_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

int main(void) {
    /* Cada pipe consome DOIS descritores, entao 600 pipes ja levam o maior fd
       acima de FD_SETSIZE. O ultimo caso existe para que o limite apareca no
       relatorio em vez de abortar o programa: FD_SET com fd >= FD_SETSIZE e
       comportamento indefinido, e o glibc com _FORTIFY_SOURCE encerra o
       processo. Um exemplo que estoura o limite que pretende ensinar nao
       ensina nada - ele so quebra. */
    int totais[] = { 32, 128, 400, 600 };
    printf("%d descritores ativos em todos os casos, %d chamadas por medicao\n",
           ATIVOS, RODADAS);
    printf("\n%-8s %14s %14s %10s\n", "total", "select (us)", "epoll (us)", "razao");

    for (size_t c = 0; c < sizeof totais / sizeof *totais; c++) {
        int n = totais[c];
        int (*p)[2] = calloc((size_t)n, sizeof *p);
        if (!p) { perror("calloc"); return 1; }

        int maior = 0;
        for (int i = 0; i < n; i++) {
            if (pipe(p[i])) { perror("pipe"); return 1; }
            if (p[i][0] > maior) maior = p[i][0];
        }
        for (int i = 0; i < ATIVOS; i++) {
            ssize_t w = write(p[i][1], "x", 1);   /* estes ficam prontos */
            (void)w;
        }

        /* --- select: o conjunto e remontado e varrido a cada chamada --- */
        if (maior >= FD_SETSIZE) {
            printf("%-8d %14s %14s %10s   maior fd = %d\n", n,
                   "FD_SETSIZE", "-", "-", maior);
            int ep0 = epoll_create1(0);
            close(ep0);
            for (int i = 0; i < n; i++) { close(p[i][0]); close(p[i][1]); }
            free(p);
            continue;
        }
        double t0 = agora_ms();
        for (int r = 0; r < RODADAS; r++) {
            fd_set leitura;
            FD_ZERO(&leitura);
            for (int i = 0; i < n; i++) FD_SET(p[i][0], &leitura);
            struct timeval zero = { 0, 0 };
            int prontos = select(maior + 1, &leitura, NULL, NULL, &zero);
            if (prontos < 0) { perror("select"); return 1; }
        }
        double sel = (agora_ms() - t0) * 1000.0 / RODADAS;

        /* --- epoll: o interesse fica no nucleo, registrado uma vez --- */
        int ep = epoll_create1(0);
        if (ep < 0) { perror("epoll_create1"); return 1; }
        for (int i = 0; i < n; i++) {
            struct epoll_event ev;
            memset(&ev, 0, sizeof ev);
            ev.events = EPOLLIN;
            ev.data.fd = p[i][0];
            if (epoll_ctl(ep, EPOLL_CTL_ADD, p[i][0], &ev)) { perror("epoll_ctl"); return 1; }
        }
        struct epoll_event saida[64];
        t0 = agora_ms();
        for (int r = 0; r < RODADAS; r++) {
            int prontos = epoll_wait(ep, saida, 64, 0);
            if (prontos < 0) { perror("epoll_wait"); return 1; }
        }
        double epo = (agora_ms() - t0) * 1000.0 / RODADAS;

        printf("%-8d %14.2f %14.2f %9.1fx\n", n, sel, epo, epo > 0 ? sel / epo : 0);

        close(ep);
        for (int i = 0; i < n; i++) { close(p[i][0]); close(p[i][1]); }
        free(p);
    }

    printf("\nO tempo do select acompanha o TOTAL; o do epoll, os PRONTOS.\n"
           "A ultima linha e o teto: FD_SETSIZE vale %d nesta maquina, e um\n"
           "descritor acima disso nao cabe no fd_set. Nao e questao de limite\n"
           "de processo: e o tipo, e nao se contorna com ulimit.\n", FD_SETSIZE);
    return 0;
}
