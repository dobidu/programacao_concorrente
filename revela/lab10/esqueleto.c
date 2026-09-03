/* LAB-10 - O pipeline atravessa a rede.   preparatório, sem nota
 *
 * O mesmo filtro, agora com o cliente de um lado e o servidor do outro. O
 * programa levanta o servidor numa thread, conecta o cliente nele e grava o
 * resultado - assim o portão continua valendo: a saída tem de bater com o
 * oráculo local, byte a byte.
 *
 * O que o exercício cobra não é socket, é ENQUADRAMENTO. TCP entrega um fluxo de
 * bytes, não mensagens: um send() de 2 MB chega como dezenas de recv() parciais,
 * e um recv() pode trazer o fim de uma mensagem junto com o começo da seguinte.
 * Quem trata `recv` como se fosse "receba uma mensagem" escreve um programa que
 * funciona com imagem pequena e quebra com imagem grande.
 *
 *   make lab10 && ./verifica.sh ./lab10/esqueleto entrada.ppm --filtro cinza
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER: o portão passando com a imagem de 1200x800 (2,7 MB - bem maior que
 * qualquer buffer de socket) e uma frase dizendo por que um recv() só não basta.
 */
#include "crono.h"
#include "filtro.h"
#include "revela.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct { uint32_t largura, altura; } Cabecalho;

/* Os dois laços que fazem o enquadramento funcionar. Sem eles, tudo o mais é
 * irrelevante: send e recv podem transferir menos do que se pediu, sempre. */
static int enviar_tudo(int fd, const void *buf, size_t n) {
    const unsigned char *p = buf;
    while (n) {
        ssize_t k = send(fd, p, n, MSG_NOSIGNAL);
        if (k < 0) { if (errno == EINTR) continue; return -1; }
        p += k; n -= (size_t)k;
    }
    return 0;
}

static int receber_tudo(int fd, void *buf, size_t n) {
    /* TODO: receba exatamente n bytes, em laço. Trate os três casos:
     *   k > 0  recebeu parte - avance e continue;
     *   k == 0 o outro lado fechou antes da hora - é erro;
     *   k < 0  erro; se for EINTR, tente de novo.
     * Um único recv(fd, buf, n, 0) funciona com imagem pequena e falha com a
     * grande. É o defeito que este exercício existe para você não escrever. */
    (void)fd; (void)buf; (void)n;
    return -1;
}

typedef struct { int escuta; Filtro filtro; } Servidor;

static void *servir(void *arg) {
    Servidor *s = arg;
    int c = accept(s->escuta, NULL, NULL);
    if (c < 0) return NULL;

    Cabecalho cab;
    if (receber_tudo(c, &cab, sizeof cab)) { close(c); return NULL; }

    Imagem entrada, saida;
    if (imagem_criar(&entrada, (int)ntohl(cab.largura), (int)ntohl(cab.altura))) {
        close(c); return NULL;
    }
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) { close(c); return NULL; }

    if (receber_tudo(c, entrada.px, imagem_bytes(&entrada))) { close(c); return NULL; }

    if (s->filtro == FILTRO_CINZA) {
        filtro_cinza(&entrada, &saida, 0, entrada.altura);
    } else {
        Imagem temp;
        if (imagem_criar(&temp, entrada.largura, entrada.altura)) { close(c); return NULL; }
        filtro_desfoque_h(&entrada, &temp, 0, entrada.altura);
        filtro_desfoque_v(&temp, &saida, 0, entrada.altura);
        imagem_destruir(&temp);
    }

    enviar_tudo(c, &cab, sizeof cab);
    enviar_tudo(c, saida.px, imagem_bytes(&saida));
    close(c);
    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    return NULL;
}

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Imagem entrada, recebida;
    if (ppm_ler(o.entrada, &entrada)) return 1;

    /* Porta 0: o sistema escolhe uma livre. Sem porta fixa, o portão pode rodar
     * 200 vezes seguidas sem esbarrar em TIME_WAIT nem em outro aluno. */
    int escuta = socket(AF_INET, SOCK_STREAM, 0);
    if (escuta < 0) { perror("socket"); return 1; }
    int um = 1;
    setsockopt(escuta, SOL_SOCKET, SO_REUSEADDR, &um, sizeof um);

    struct sockaddr_in ep;
    memset(&ep, 0, sizeof ep);
    ep.sin_family = AF_INET;
    ep.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ep.sin_port = 0;
    if (bind(escuta, (struct sockaddr *)&ep, sizeof ep)) { perror("bind"); return 1; }
    if (listen(escuta, 4)) { perror("listen"); return 1; }

    socklen_t tam = sizeof ep;
    if (getsockname(escuta, (struct sockaddr *)&ep, &tam)) { perror("getsockname"); return 1; }

    Servidor s = { escuta, o.filtro };
    pthread_t fio;
    if (pthread_create(&fio, NULL, servir, &s)) { perror("pthread_create"); return 1; }

    double t0 = agora_ms();

    int c = socket(AF_INET, SOCK_STREAM, 0);
    if (c < 0) { perror("socket"); return 1; }
    if (connect(c, (struct sockaddr *)&ep, sizeof ep)) { perror("connect"); return 1; }

    Cabecalho cab = { htonl((uint32_t)entrada.largura), htonl((uint32_t)entrada.altura) };
    if (enviar_tudo(c, &cab, sizeof cab)) { perror("send"); return 1; }
    if (enviar_tudo(c, entrada.px, imagem_bytes(&entrada))) { perror("send"); return 1; }

    Cabecalho volta;
    if (receber_tudo(c, &volta, sizeof volta)) { perror("recv"); return 1; }
    if (imagem_criar(&recebida, (int)ntohl(volta.largura), (int)ntohl(volta.altura))) return 1;
    if (receber_tudo(c, recebida.px, imagem_bytes(&recebida))) { perror("recv"); return 1; }
    close(c);

    pthread_join(fio, NULL);
    close(escuta);
    double ms = agora_ms() - t0;

    unsigned long baldes[256];
    memset(baldes, 0, sizeof baldes);
    histograma_faixa(&recebida, 0, recebida.altura, baldes);

    int erro = ppm_escrever(o.saida, &recebida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr, "porta=%d  ida e volta de %zu KB em %.2f ms\n",
            ntohs(ep.sin_port), imagem_bytes(&entrada) / 1024, ms);

    imagem_destruir(&entrada);
    imagem_destruir(&recebida);
    return erro ? 1 : 0;
}
