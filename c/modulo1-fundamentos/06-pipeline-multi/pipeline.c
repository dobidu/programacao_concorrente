/*
 * pipeline.c — Pipeline multi-estágio: ls -l | grep ".c" | wc -l
 *
 * Conceitos: múltiplos pipes, dup2(), gerenciamento de descritores
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int pipe1[2], pipe2[2];
    /* O retorno de pipe() é verificado, e não ignorado: sem descritor não há
       pipeline, e seguir adiante produziria dup2 sobre lixo. O portão do
       projeto compila com -Werror, e -Wunused-result acusa exatamente isto. */
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {  /* ls -> grep -> wc */
        perror("pipe");
        return 1;
    }

    /* Processo 1: ls -l (stdout → pipe1) */
    if (fork() == 0) {
        close(pipe1[0]); close(pipe2[0]); close(pipe2[1]);
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[1]);
        execlp("ls", "ls", "-l", NULL);
        _exit(1);
    }

    /* Processo 2: grep ".c" (stdin ← pipe1, stdout → pipe2) */
    if (fork() == 0) {
        close(pipe1[1]); close(pipe2[0]);
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe1[0]); close(pipe2[1]);
        execlp("grep", "grep", "\\.c", NULL);
        _exit(1);
    }

    /* Processo 3: wc -l (stdin ← pipe2) */
    if (fork() == 0) {
        close(pipe1[0]); close(pipe1[1]); close(pipe2[1]);
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        execlp("wc", "wc", "-l", NULL);
        _exit(1);
    }

    /* Pai: fecha TODOS os descritores e espera */
    close(pipe1[0]); close(pipe1[1]);
    close(pipe2[0]); close(pipe2[1]);
    for (int i = 0; i < 3; i++) wait(NULL);

    return 0;
}
