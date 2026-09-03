/* LAB-12 - SOLUÇÃO de referência: o mesmo servidor em uma thread, com epoll.
 *
 * O LAB-11 mostrou o thread-por-cliente saturando: a vazão empaca e a latência
 * cresce junto com o número de clientes, porque a máquina passa a gastar tempo
 * trocando de contexto.
 *
 * Aqui o servidor inteiro é UMA thread. Em vez de bloquear num recv por conexão,
 * ele pergunta ao kernel quais descritores estão prontos e atende só esses. É o
 * padrão reator, e é o que está por baixo de nginx, redis e libuv.
 *
 * Duas coisas que o exercício cobra:
 *
 *   · sockets NÃO-BLOQUEANTES. Num laço de eventos, um read que bloqueia trava o
 *     servidor inteiro - não uma conexão, todas. EAGAIN deixa de ser erro e passa
 *     a ser a resposta normal para "não tem mais dado agora".
 *
 *   · ESTADO POR CONEXÃO. Sem thread não há pilha para guardar "estou no meio de
 *     receber o cabeçalho": esse progresso vira campo de uma struct. É a inversão
 *     que o padrão cobra - a pilha da thread vira máquina de estados explícita.
 *
 * O resultado medido nesta máquina: o epoll PERDE para o thread-por-cliente aqui
 * (1300 ms contra 740 ms de p50, com 128 clientes). O reator barateia a conexão,
 * não o trabalho: uma thread usa um núcleo de 24. É a condição de contorno que a
 * frase "epoll é mais rápido" costuma omitir.
 *
 *   make lab12
 *   for n in 1 4 16 64 128; do
 *       ./lab12/solucao entrada.ppm /tmp/s.ppm --clientes $n 2>&1 | grep latencia
 *   done
 */
#include "crono.h"
#include "filtro.h"
#include "revela.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct { uint32_t largura, altura; } Cabecalho;

static int enviar_tudo(int fd, const void *buf, size_t n) {
    const unsigned char *p = buf;
    while (n) {
        ssize_t k = send(fd, p, n, MSG_NOSIGNAL);
        if (k < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;  /* ver nota */
            return -1;
        }
        p += k; n -= (size_t)k;
    }
    return 0;
}

/* O cliente é bloqueante e simples: quem precisa do laço de eventos é o servidor,
 * que atende muitos ao mesmo tempo. Um cliente atende a si mesmo. */
static int receber_tudo(int fd, void *buf, size_t n) {
    unsigned char *p = buf;
    while (n) {
        ssize_t k = recv(fd, p, n, 0);
        if (k < 0) { if (errno == EINTR) continue; return -1; }
        if (k == 0) return -1;
        p += k; n -= (size_t)k;
    }
    return 0;
}

static int nao_bloqueante(int fd) {
    int f = fcntl(fd, F_GETFL, 0);
    return f < 0 ? -1 : fcntl(fd, F_SETFL, f | O_NONBLOCK);
}

/* O estado que, num servidor com thread, moraria na pilha dela. */
typedef enum { LENDO_CABECALHO, LENDO_PIXELS, RESPONDENDO } Fase;

typedef struct Conexao {
    int fd;
    Fase fase;
    Cabecalho cab;
    size_t lidos;               /* quanto já entrou, na fase atual */
    Imagem entrada, saida;
    struct Conexao *proxima;    /* lista simples: o material não precisa de hash */
} Conexao;

typedef struct { int escuta; Filtro filtro; int quantos; } Servidor;

static Conexao *conexao_nova(int fd) {
    Conexao *c = calloc(1, sizeof *c);
    if (!c) return NULL;
    c->fd = fd;
    c->fase = LENDO_CABECALHO;
    return c;
}

static void conexao_fechar(Conexao *c, int epfd) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, NULL);
    close(c->fd);
    imagem_destruir(&c->entrada);
    imagem_destruir(&c->saida);
    free(c);
}

/* Um passo de progresso na conexão. Devolve 0 se ainda há trabalho, 1 se terminou
 * e -1 em erro. Nunca bloqueia: EAGAIN devolve 0 e o laço volta depois. */
static int conexao_avancar(Conexao *c, Filtro filtro) {
    if (c->fase == LENDO_CABECALHO) {
        ssize_t k = recv(c->fd, (unsigned char *)&c->cab + c->lidos,
                         sizeof c->cab - c->lidos, 0);
        if (k < 0) return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -1;
        if (k == 0) return -1;
        c->lidos += (size_t)k;
        if (c->lidos < sizeof c->cab) return 0;

        if (imagem_criar(&c->entrada, (int)ntohl(c->cab.largura),
                         (int)ntohl(c->cab.altura))) return -1;
        if (imagem_criar(&c->saida, c->entrada.largura, c->entrada.altura)) return -1;
        c->lidos = 0;
        c->fase = LENDO_PIXELS;
        return 0;
    }

    if (c->fase == LENDO_PIXELS) {
        size_t total = imagem_bytes(&c->entrada);
        ssize_t k = recv(c->fd, c->entrada.px + c->lidos, total - c->lidos, 0);
        if (k < 0) return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -1;
        if (k == 0) return -1;
        c->lidos += (size_t)k;
        if (c->lidos < total) return 0;

        if (filtro == FILTRO_CINZA) {
            filtro_cinza(&c->entrada, &c->saida, 0, c->entrada.altura);
        } else {
            Imagem temp;
            if (imagem_criar(&temp, c->entrada.largura, c->entrada.altura)) return -1;
            filtro_desfoque_h(&c->entrada, &temp, 0, c->entrada.altura);
            filtro_desfoque_v(&temp, &c->saida, 0, c->entrada.altura);
            imagem_destruir(&temp);
        }
        c->fase = RESPONDENDO;
        /* Sem `return` aqui, de propósito: os pixels acabaram de chegar, então
         * não virá outro EPOLLIN nesta conexão. Uma máquina de estados que
         * parasse esperando um evento que já aconteceu deixaria o servidor
         * pendurado - e o cliente junto. Cair direto na resposta fecha o ciclo.
         * É a armadilha clássica de laço de eventos: esperar por um evento de
         * borda que não vai se repetir. */
    }

    /* A resposta usa envio bloqueante por simplicidade: escrever a saída também
     * como máquina de estados é o passo natural seguinte, e fica como leitura. */
    if (enviar_tudo(c->fd, &c->cab, sizeof c->cab)) return -1;
    if (enviar_tudo(c->fd, c->saida.px, imagem_bytes(&c->saida))) return -1;
    return 1;
}

static void *servir(void *arg) {
    Servidor *s = arg;
    int epfd = epoll_create1(0);
    if (epfd < 0) return NULL;

    /* O socket de ESCUTA também precisa ser não-bloqueante. Sem isto, o laço que
     * drena a fila de aceite bloqueia no accept seguinte ao último cliente - e o
     * servidor inteiro para ali, sem nunca voltar ao epoll_wait. É o erro mais
     * comum na primeira vez que se escreve um reator, e ele não dá mensagem
     * nenhuma: só trava. */
    if (nao_bloqueante(s->escuta)) { close(epfd); return NULL; }

    struct epoll_event ev = { .events = EPOLLIN, .data.ptr = NULL };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, s->escuta, &ev)) { close(epfd); return NULL; }

    int atendidos = 0;
    struct epoll_event eventos[64];

    while (atendidos < s->quantos) {
        int n = epoll_wait(epfd, eventos, 64, 5000);
        if (n < 0) { if (errno == EINTR) continue; break; }
        if (n == 0) break;                          /* ninguém veio: desiste */

        for (int i = 0; i < n; i++) {
            Conexao *c = eventos[i].data.ptr;

            if (c == NULL) {                        /* evento no socket de escuta */
                for (;;) {
                    int novo = accept(s->escuta, NULL, NULL);
                    if (novo < 0) break;            /* drenou a fila de aceite */
                    if (nao_bloqueante(novo)) { close(novo); continue; }
                    Conexao *nc = conexao_nova(novo);
                    if (!nc) { close(novo); continue; }
                    struct epoll_event e = { .events = EPOLLIN, .data.ptr = nc };
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, novo, &e)) { conexao_fechar(nc, epfd); }
                }
                continue;
            }

            int r = conexao_avancar(c, s->filtro);
            if (r != 0) {
                conexao_fechar(c, epfd);
                if (r == 1) atendidos++;
            }
        }
    }
    close(epfd);
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
