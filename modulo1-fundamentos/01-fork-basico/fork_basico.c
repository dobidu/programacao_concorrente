/*
 * fork_basico.c — Demonstração fundamental de fork()
 *
 * Conceitos: fork(), getpid(), getppid(), cópias independentes de variáveis
 *
 * Compilar: gcc -Wall -Wextra -o fork_basico fork_basico.c
 * Executar: ./fork_basico
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int x = 10;  /* Variável local — será copiada no fork */

    printf("Antes do fork: PID = %d, x = %d\n", getpid(), x);

    pid_t pid = fork();  /* Ponto de bifurcação! */

    if (pid < 0) {
        perror("fork falhou");
        return 1;
    }
    else if (pid == 0) {
        /* ── Código do FILHO ── */
        x = 20;  /* Modifica SUA cópia de x */
        printf("Filho:  PID=%d, PPID=%d, x=%d\n", getpid(), getppid(), x);
    }
    else {
        /* ── Código do PAI ── */
        x = 30;  /* Modifica SUA cópia de x */
        printf("Pai:    PID=%d, FilhoPID=%d, x=%d\n", getpid(), pid, x);
        wait(NULL);  /* Espera filho terminar (evita zombie) */
    }

    /* Executado por AMBOS os processos */
    printf("PID=%d: x=%d (fim)\n", getpid(), x);
    return 0;
}
