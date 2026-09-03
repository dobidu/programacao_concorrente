/*
 * fork_multiplos.c — Criação de múltiplos processos filhos
 *
 * Conceitos: fork() em loop, _exit() para evitar fork bomb, waitpid()
 *
 * Compilar: gcc -Wall -Wextra -o fork_multiplos fork_multiplos.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_FILHOS 5

int main() {
    printf("Pai (PID %d) criando %d filhos...\n", getpid(), NUM_FILHOS);

    for (int i = 0; i < NUM_FILHOS; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            /* ── FILHO: executa trabalho e TERMINA ── */
            printf("  Filho %d: PID=%d, calculando...\n", i, getpid());
            long soma = 0;
            for (long j = 1; j <= (i + 1) * 100000L; j++)
                soma += j;
            printf("  Filho %d: soma(1..%d) = %ld\n", i, (i+1)*100000, soma);
            _exit(0);  /* FUNDAMENTAL: terminar o filho aqui! */
        }
        /* Pai continua o loop — filho já saiu com _exit */
    }

    /* Pai espera TODOS os filhos */
    printf("Pai esperando todos os filhos...\n");
    for (int i = 0; i < NUM_FILHOS; i++) {
        int status;
        pid_t child = wait(&status);
        if (WIFEXITED(status))
            printf("  Filho PID=%d terminou (código %d)\n",
                   child, WEXITSTATUS(status));
    }

    printf("Todos os filhos terminaram.\n");
    return 0;
}
