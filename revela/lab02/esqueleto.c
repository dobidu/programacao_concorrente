/* LAB-02 - O mesmo filtro em dois processos, por pipe.   preparatório, sem nota
 *
 * Dois processos ligados por um pipe, como `A | B` no shell. O pai lê a imagem e
 * empurra linha a linha; o filho filtra e escreve o arquivo de saída.
 *
 * Três coisas que o exercício cobra e que costumam faltar:
 *   · fechar a ponta que não se usa - sem isso o leitor nunca vê o fim de arquivo;
 *   · escrever e ler em laço - write() e read() podem transferir menos que o pedido;
 *   · recolher o filho com waitpid() - sem isso ele vira zumbi.
 *
 * Como vem, o esqueleto TRAVA. Isso é de propósito: descubra onde, antes de
 * consertar. Em outro terminal, com o programa pendurado:
 *
 *     gdb -p $(pgrep -n -f lab02/esqueleto)
 *     (gdb) thread apply all bt        # o pai está em waitpid
 *     (gdb) detach
 *     gdb -p <pid do filho>
 *     (gdb) bt                         # o filho está em read
 *
 * Um espera o outro terminar; o outro espera um fim de arquivo que ninguém
 * manda. É espera circular, com dois processos em vez de duas threads.
 *
 *   make lab02 && ./verifica.sh ./lab02/esqueleto entrada.ppm --filtro cinza
 *   ps -eo pid,stat,comm | grep -w Z        # depois do conserto: vazio
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER: o portão passando, nenhum zumbi, e uma frase dizendo qual fecho
 * faltava e por que a falta dele trava o LEITOR.
 */
#include "crono.h"
#include "filtro.h"
#include "revela.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* write() pode escrever menos que o pedido. Só um laço garante tudo. */
static int escrever_tudo(int fd, const void *buf, size_t n) {
    const unsigned char *p = buf;
    while (n > 0) {
        ssize_t k = write(fd, p, n);
        if (k < 0) { if (errno == EINTR) continue; return -1; }
        p += k; n -= (size_t)k;
    }
    return 0;
}

/* Devolve 1 se leu a linha inteira, 0 no fim de arquivo limpo, -1 em erro.
 *
 * O laço termina no fim de arquivo, e não numa contagem de linhas: é isso que
 * torna o programa dependente de alguém fechar a ponta de escrita. Um leitor que
 * conta linhas esconde o defeito; este não esconde. */
static int ler_linha(int fd, void *buf, size_t n) {
    unsigned char *p = buf;
    size_t restante = n;
    while (restante > 0) {
        ssize_t k = read(fd, p, restante);
        if (k < 0) { if (errno == EINTR) continue; return -1; }
        if (k == 0) return restante == n ? 0 : -1;   /* EOF; parcial é erro */
        p += k; restante -= (size_t)k;
    }
    return 1;
}

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Imagem entrada;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    size_t bytes_linha = (size_t)entrada.largura * 3;

    int tubo[2];
    if (pipe(tubo)) { perror("pipe"); return 1; }

    pid_t filho = fork();
    if (filho < 0) { perror("fork"); return 1; }

    if (filho == 0) {
        /* ── filho: filtra o que chega e escreve a saída ── */
        /* TODO: feche aqui a ponta do pipe que o filho não usa. */

        Imagem saida;
        if (imagem_criar(&saida, entrada.largura, entrada.altura)) _exit(1);
        unsigned long baldes[256];
        memset(baldes, 0, sizeof baldes);

        Imagem linha = { entrada.largura, 1, NULL };
        Imagem linha_out = { entrada.largura, 1, NULL };
        linha.px = malloc(bytes_linha);
        linha_out.px = malloc(bytes_linha);
        if (!linha.px || !linha_out.px) _exit(1);

        int y = 0, r;
        while ((r = ler_linha(tubo[0], linha.px, bytes_linha)) == 1) {
            if (y >= entrada.altura) _exit(1);
            filtro_cinza(&linha, &linha_out, 0, 1);
            memcpy(saida.px + (size_t)y * bytes_linha, linha_out.px, bytes_linha);
            histograma_faixa(&linha_out, 0, 1, baldes);
            y++;
        }
        if (r < 0 || y != entrada.altura) _exit(1);
        close(tubo[0]);

        int erro = ppm_escrever(o.saida, &saida);
        imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
        fflush(stdout);
        _exit(erro ? 1 : 0);                /* _exit: não roda atexit do pai */
    }

    /* ── pai: empurra a imagem linha a linha ── */
    /* TODO: feche aqui a ponta do pipe que o pai não usa. */
    double t0 = agora_ms();
    for (int y = 0; y < entrada.altura; y++) {
        if (escrever_tudo(tubo[1], entrada.px + (size_t)y * bytes_linha, bytes_linha)) {
            perror("write"); break;
        }
    }
    /* TODO: falta uma linha aqui. Sem ela o programa trava - e travar é o
     *       resultado esperado do esqueleto, não um acidente. */

    int estado = 0;
    if (waitpid(filho, &estado, 0) < 0) { perror("waitpid"); return 1; }
    fprintf(stderr, "tempo %.2f ms\n", agora_ms() - t0);

    imagem_destruir(&entrada);
    return WIFEXITED(estado) && WEXITSTATUS(estado) == 0 ? 0 : 1;
}
