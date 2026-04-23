/*
 * MiniChat v0.1 — Comunicação pai→filho via pipe
 * Conceitos: fork, pipe, read/write
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0) {
        close(pipefd[1]);
        char buf[256];
        ssize_t n;
        printf("[CHAT] Aguardando mensagens...\n");
        while ((n = read(pipefd[0], buf, sizeof(buf)-1)) > 0) {
            buf[n] = '\0';
            printf("[CHAT] %s", buf);
        }
        close(pipefd[0]);
        _exit(0);
    }

    close(pipefd[0]);
    char msg[256];
    printf("MiniChat v0.1 — Digite mensagens (Ctrl+D sair):\n");
    while (fgets(msg, sizeof(msg), stdin))
        write(pipefd[1], msg, strlen(msg));
    close(pipefd[1]);
    wait(NULL);
    printf("Chat encerrado.\n");
    return 0;
}
