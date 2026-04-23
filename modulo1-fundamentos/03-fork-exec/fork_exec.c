/*
 * fork_exec.c — Padrão fork-exec (como shells funcionam)
 *
 * Conceitos: fork()+execlp(), substituição de programa, WEXITSTATUS
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("=== Executando 'ls -la /tmp' via fork+exec ===\n\n");

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Filho: substitui-se por 'ls' */
        execlp("ls", "ls", "-la", "/tmp", NULL);
        /* Se exec retornar, houve erro */
        perror("exec falhou");
        _exit(127);
    }

    /* Pai: espera e coleta status */
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        printf("\nComando terminou com código %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("\nComando morto por sinal %d\n", WTERMSIG(status));

    return 0;
}
