/*
 * pipe_basico.c — Comunicação pai→filho via pipe
 *
 * Conceitos: pipe(), read/write, fechar descritores não usados, EOF
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];  /* [0]=leitura, [1]=escrita */

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* ── FILHO: lê do pipe ── */
        close(pipefd[1]);  /* Fecha escrita (não precisa) */

        char buf[256];
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("[Filho recebeu] %s", buf);
        }
        printf("[Filho] EOF recebido, encerrando.\n");

        close(pipefd[0]);
        _exit(0);
    } else {
        /* ── PAI: escreve no pipe ── */
        close(pipefd[0]);  /* Fecha leitura (não precisa) */

        const char *msgs[] = {
            "Primeira mensagem\n",
            "Segunda mensagem\n",
            "Terceira e última\n",
        };
        for (int i = 0; i < 3; i++) {
            write(pipefd[1], msgs[i], strlen(msgs[i]));
            usleep(200000);  /* 200ms entre mensagens */
        }

        close(pipefd[1]);  /* Fecha escrita → filho recebe EOF */
        wait(NULL);
    }

    return 0;
}
