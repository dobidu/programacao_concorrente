/* sinais.c - sigaction, handler async-signal-safe, EINTR e máscara.
 *
 * Três afirmações da página de sinais, todas verificáveis aqui:
 *
 *   1. o handler executa entre duas instruções quaisquer, então quase nada é
 *      seguro lá dentro. A lista do que é vale está em signal-safety(7), e
 *      escrever um `volatile sig_atomic_t` está nela; printf NÃO está;
 *   2. a chamada bloqueante interrompida volta com -1 e errno igual a EINTR,
 *      sem ter feito o trabalho, e tratar todo -1 como erro fatal produz um
 *      programa que funciona até alguém mandar um sinal;
 *   3. a pendência de sinal padrão é um BIT e não um contador: dois envios com
 *      o sinal bloqueado resultam numa única execução do handler.
 *
 *     gcc -Wall -Wextra -pthread sinais.c -o sinais
 *     ./sinais
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t chegou;     /* o único tipo seguro no handler */
static volatile sig_atomic_t execucoes;

static void handler(int sig) {
    (void)sig;
    chegou = 1;
    execucoes += 1;                      /* e nada além disto */
}

/* Demonstra 2: read é interrompido e devolve -1 com EINTR. O laço de repetição
 * é a correção que funciona sempre; SA_RESTART é a que funciona para as
 * chamadas reiniciáveis, e nem toda chamada é. */
static ssize_t ler_repetindo(int fd, void *buf, size_t n, int *interrupcoes) {
    for (;;) {
        ssize_t r = read(fd, buf, n);
        if (r >= 0) return r;
        if (errno == EINTR) { (*interrupcoes)++; continue; }
        return -1;
    }
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                     /* SEM SA_RESTART: queremos ver EINTR */
    if (sigaction(SIGUSR1, &sa, NULL)) { perror("sigaction"); return 1; }

    /* --- 3: dois envios com o sinal bloqueado, um handler --- */
    sigset_t so_usr1, anterior;
    sigemptyset(&so_usr1);
    sigaddset(&so_usr1, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &so_usr1, &anterior)) { perror("sigprocmask"); return 1; }

    raise(SIGUSR1);
    raise(SIGUSR1);                      /* o segundo some: pendencia e um bit */

    sigset_t pendentes;
    sigpending(&pendentes);
    printf("com o sinal bloqueado: pendente=%d, handler executou %dx\n",
           sigismember(&pendentes, SIGUSR1), (int)execucoes);

    if (sigprocmask(SIG_SETMASK, &anterior, NULL)) { perror("sigprocmask"); return 1; }
    printf("apos desbloquear:      handler executou %dx para 2 envios\n",
           (int)execucoes);

    /* --- 2: EINTR num read que espera de um pipe vazio --- */
    int p[2];
    if (pipe(p)) { perror("pipe"); return 1; }

    pid_t filho = fork();
    if (filho < 0) { perror("fork"); return 1; }
    if (filho == 0) {
        close(p[0]);
        /* interrompe o pai duas vezes e so entao escreve. nanosleep, e nao
           usleep: usleep exige _XOPEN_SOURCE e este arquivo declara apenas
           _POSIX_C_SOURCE, entao usa-lo produziria declaracao implicita. */
        struct timespec pausa = { 0, 120000000L };
        for (int i = 0; i < 2; i++) { nanosleep(&pausa, NULL); kill(getppid(), SIGUSR1); }
        nanosleep(&pausa, NULL);
        ssize_t ignorado = write(p[1], "x", 1);
        (void)ignorado;
        close(p[1]);
        _exit(0);
    }

    close(p[1]);
    char c;
    int interrupcoes = 0;
    ssize_t r = ler_repetindo(p[0], &c, 1, &interrupcoes);
    printf("read devolveu %zd apos %d interrupcao(oes) por EINTR\n", r, interrupcoes);
    printf("flag do handler: %d\n", (int)chegou);

    close(p[0]);
    return 0;
}
