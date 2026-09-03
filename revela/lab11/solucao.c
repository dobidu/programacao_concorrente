/* LAB-11 - SOLUÇÃO de referência: onde o thread-por-cliente joelha.
 *
 * O modelo mais simples de servidor concorrente: uma thread por conexão. É o que
 * o LAB-10 fez com um cliente só, agora com N ao mesmo tempo.
 *
 * O modelo funciona, e funciona bem - até certo ponto. Cada thread custa pilha
 * (8 MB de espaço virtual por padrão), custa uma entrada no escalonador, e cada
 * troca entre elas custa tempo. Com dezenas de clientes isso não aparece. Com
 * centenas, a máquina passa mais tempo trocando de contexto do que filtrando
 * imagem, e a latência sobe mais rápido que o número de clientes.
 *
 * O exercício é achar o joelho da curva NESTA máquina, e dizer o que o causa.
 *
 *   make lab11
 *   for n in 1 2 4 8 16 32 64 128; do
 *       ./lab11/solucao entrada.ppm /tmp/s.ppm --clientes $n 2>&1 | grep latencia
 *   done
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
    unsigned char *p = buf;
    while (n) {
        ssize_t k = recv(fd, p, n, 0);
        if (k < 0) { if (errno == EINTR) continue; return -1; }
        if (k == 0) return -1;                  /* o outro lado fechou antes */
        p += k; n -= (size_t)k;
    }
    return 0;
}

typedef struct { int escuta; Filtro filtro; int quantos; } Servidor;

/* Uma thread por conexão aceita: o laço de accept só despacha. */
typedef struct { int conexao; Filtro filtro; } Atendimento;

static void *atender(void *arg) {
    Atendimento *a = arg;
    int c = a->conexao;
    Filtro filtro = a->filtro;
    free(a);

    Cabecalho cab;
    if (receber_tudo(c, &cab, sizeof cab)) { close(c); return NULL; }

    Imagem entrada, saida;
    if (imagem_criar(&entrada, (int)ntohl(cab.largura), (int)ntohl(cab.altura))) {
        close(c); return NULL;
    }
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) { close(c); return NULL; }

    if (receber_tudo(c, entrada.px, imagem_bytes(&entrada))) { close(c); return NULL; }

    if (filtro == FILTRO_CINZA) {
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

static void *servir(void *arg) {
    Servidor *s = arg;
    for (int i = 0; i < s->quantos; i++) {
        int c = accept(s->escuta, NULL, NULL);
        if (c < 0) break;
        Atendimento *a = malloc(sizeof *a);
        if (!a) { close(c); break; }
        *a = (Atendimento){ c, s->filtro };
        pthread_t f;
        if (pthread_create(&f, NULL, atender, a)) { free(a); close(c); break; }
        pthread_detach(f);      /* ninguém junta: o atendimento é independente */
    }
    return NULL;
}

typedef struct {
    struct sockaddr_in ep;
    const Imagem *entrada;
    Imagem *destino;        /* só o cliente 0 preenche: a saída tem de ser única */
    double latencia_ms;
    int ok;
} Cliente;

static void *cliente(void *arg) {
    Cliente *cl = arg;
    double t0 = agora_ms();

    int c = socket(AF_INET, SOCK_STREAM, 0);
    if (c < 0) return NULL;
    if (connect(c, (struct sockaddr *)&cl->ep, sizeof cl->ep)) { close(c); return NULL; }

    Cabecalho cab = { htonl((uint32_t)cl->entrada->largura),
                      htonl((uint32_t)cl->entrada->altura) };
    if (enviar_tudo(c, &cab, sizeof cab) ||
        enviar_tudo(c, cl->entrada->px, imagem_bytes(cl->entrada))) { close(c); return NULL; }

    Cabecalho volta;
    if (receber_tudo(c, &volta, sizeof volta)) { close(c); return NULL; }

    Imagem recebida;
    if (imagem_criar(&recebida, (int)ntohl(volta.largura), (int)ntohl(volta.altura))) {
        close(c); return NULL;
    }
    if (receber_tudo(c, recebida.px, imagem_bytes(&recebida))) {
        imagem_destruir(&recebida); close(c); return NULL;
    }
    close(c);

    if (cl->destino) {
        memcpy(cl->destino->px, recebida.px, imagem_bytes(&recebida));
    }
    imagem_destruir(&recebida);

    cl->latencia_ms = agora_ms() - t0;
    cl->ok = 1;
    return NULL;
}

static int comparar(const void *a, const void *b) {
    double x = ((const Cliente *)a)->latencia_ms, y = ((const Cliente *)b)->latencia_ms;
    return x < y ? -1 : x > y ? 1 : 0;
}

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Imagem entrada, saida;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;

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
    if (listen(escuta, 512)) { perror("listen"); return 1; }
    socklen_t tam = sizeof ep;
    if (getsockname(escuta, (struct sockaddr *)&ep, &tam)) { perror("getsockname"); return 1; }

    Servidor s = { escuta, o.filtro, o.clientes };
    pthread_t fio_servidor;
    if (pthread_create(&fio_servidor, NULL, servir, &s)) { perror("pthread_create"); return 1; }

    pthread_t *fios = calloc((size_t)o.clientes, sizeof *fios);
    Cliente *cls = calloc((size_t)o.clientes, sizeof *cls);
    if (!fios || !cls) { perror("calloc"); return 1; }

    double t0 = agora_ms();
    for (int i = 0; i < o.clientes; i++) {
        cls[i].ep = ep;
        cls[i].entrada = &entrada;
        cls[i].destino = (i == 0) ? &saida : NULL;
        if (pthread_create(&fios[i], NULL, cliente, &cls[i])) { perror("pthread_create"); return 1; }
    }
    for (int i = 0; i < o.clientes; i++) pthread_join(fios[i], NULL);
    double total = agora_ms() - t0;
    pthread_join(fio_servidor, NULL);
    close(escuta);

    int falhas = 0;
    for (int i = 0; i < o.clientes; i++) if (!cls[i].ok) falhas++;
    if (falhas) { fprintf(stderr, "ERRO: %d cliente(s) falharam\n", falhas); return 1; }

    qsort(cls, (size_t)o.clientes, sizeof *cls, comparar);
    double p50 = cls[o.clientes / 2].latencia_ms;
    double p95 = cls[(int)((o.clientes - 1) * 0.95)].latencia_ms;
    double pior = cls[o.clientes - 1].latencia_ms;

    unsigned long baldes[256];
    memset(baldes, 0, sizeof baldes);
    histograma_faixa(&saida, 0, saida.altura, baldes);

    int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr, "latencia clientes=%d p50=%.1f ms p95=%.1f ms pior=%.1f ms "
                    "total=%.1f ms vazao=%.1f img/s\n",
            o.clientes, p50, p95, pior, total,
            total > 0 ? o.clientes * 1000.0 / total : 0.0);

    free(fios); free(cls);
    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    return erro ? 1 : 0;
}
