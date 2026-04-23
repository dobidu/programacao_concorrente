/*
 * pipe_bidi.c — Comunicação bidirecional com dois pipes
 *
 * Conceitos: 2 pipes para pai↔filho, protocolo request-response
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>

int main() {
    int pai_para_filho[2], filho_para_pai[2];
    pipe(pai_para_filho);
    pipe(filho_para_pai);

    if (fork() == 0) {
        /* ── FILHO: lê do pai, converte para maiúsculas, devolve ── */
        close(pai_para_filho[1]);
        close(filho_para_pai[0]);

        char buf[256];
        ssize_t n;
        while ((n = read(pai_para_filho[0], buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            /* Converte para maiúsculas */
            for (int i = 0; i < n; i++)
                buf[i] = toupper((unsigned char)buf[i]);
            write(filho_para_pai[1], buf, n);
        }

        close(pai_para_filho[0]);
        close(filho_para_pai[1]);
        _exit(0);
    }

    /* ── PAI: envia string, recebe versão em maiúsculas ── */
    close(pai_para_filho[0]);
    close(filho_para_pai[1]);

    const char *msg = "hello world from pipe!\n";
    printf("Pai enviou:  %s", msg);
    write(pai_para_filho[1], msg, strlen(msg));
    close(pai_para_filho[1]);  /* Sinaliza EOF */

    char resp[256];
    ssize_t n = read(filho_para_pai[0], resp, sizeof(resp) - 1);
    resp[n] = '\0';
    printf("Pai recebeu: %s", resp);

    close(filho_para_pai[0]);
    wait(NULL);
    return 0;
}
