/* http_cru.c - onde termina uma resposta HTTP, sobre um fluxo de bytes.
 *
 * O HTTP não acrescenta mecanismo nenhum: é texto sobre TCP. Toda a dificuldade
 * de escrever um cliente está numa pergunta, que é a da página do UDP vista do
 * outro lado - como saber que a mensagem terminou.
 *
 * Sobre um fluxo essa informação não existe: o TCP entrega bytes e não sabe o
 * que é uma resposta. O protocolo precisa dizer, e o HTTP diz de três formas: a
 * linha em branco termina o cabeçalho, o Content-Length delimita o corpo, e o
 * Transfer-Encoding chunked delimita quando o tamanho não é conhecido antes.
 *
 * O programa sobe um servidor mínimo em loopback, numa thread, e roda dois
 * clientes contra ele: um que respeita o Content-Length e um que lê até o fim
 * da conexão. O segundo pendura, porque o HTTP/1.1 tem keep-alive por padrão e
 * a estratégia depende exatamente de o servidor fechar.
 *
 *     gcc -O2 -Wall -Wextra -pthread http_cru.c -o http_cru
 *     ./http_cru
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define CORPO 300

static int porta_pronta;
static unsigned short porta;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

static void *servidor(void *a) {
    (void)a;
    int escuta = socket(AF_INET, SOCK_STREAM, 0);
    int um = 1;
    setsockopt(escuta, SOL_SOCKET, SO_REUSEADDR, &um, sizeof um);

    struct sockaddr_in end;
    memset(&end, 0, sizeof end);
    end.sin_family = AF_INET;
    end.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    end.sin_port = 0;                       /* porta efemera */
    bind(escuta, (struct sockaddr *)&end, sizeof end);
    listen(escuta, 8);

    socklen_t tam = sizeof end;
    getsockname(escuta, (struct sockaddr *)&end, &tam);

    pthread_mutex_lock(&m);
    porta = ntohs(end.sin_port);
    porta_pronta = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&m);

    for (int i = 0; i < 2; i++) {
        int c = accept(escuta, NULL, NULL);
        if (c < 0) continue;

        char pedido[1024];
        ssize_t r = recv(c, pedido, sizeof pedido - 1, 0);
        if (r > 0) pedido[r] = '\0';

        char cab[256];
        int n = snprintf(cab, sizeof cab,
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: %d\r\n"
                         "\r\n", CORPO);
        ssize_t w = send(c, cab, (size_t)n, 0);
        (void)w;

        /* o corpo sai em DOIS pedaços, de proposito: um send nao corresponde
           a um recv, e o cliente precisa acumular */
        char corpo[CORPO];
        memset(corpo, 'a', sizeof corpo);
        w = send(c, corpo, 180, 0);
        usleep(30000);
        w = send(c, corpo + 180, CORPO - 180, 0);
        (void)w;

        /* keep-alive: NAO se fecha a conexao */
        if (i == 1) { sleep(1); close(c); }   /* o segundo cliente e liberado por timeout */
        else        { close(c); }             /* o primeiro ja saiu sozinho */
    }
    close(escuta);
    return NULL;
}

static int conectar(void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in end;
    memset(&end, 0, sizeof end);
    end.sin_family = AF_INET;
    end.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    end.sin_port = htons(porta);
    if (connect(s, (struct sockaddr *)&end, sizeof end)) { perror("connect"); return -1; }
    return s;
}

static void pedir(int s) {
    const char *req = "GET /a HTTP/1.1\r\nHost: localhost\r\n\r\n";
    ssize_t w = send(s, req, strlen(req), 0);
    (void)w;
}

int main(void) {
    pthread_t t;
    pthread_create(&t, NULL, servidor, NULL);

    pthread_mutex_lock(&m);
    while (!porta_pronta) pthread_cond_wait(&cv, &m);
    pthread_mutex_unlock(&m);

    /* --- cliente 1: respeita o Content-Length --- */
    int s = conectar();
    pedir(s);

    char buf[4096];
    size_t total = 0;
    int declarado = -1, leituras = 0;
    size_t corpo_lido = 0, cabecalho = 0;
    for (;;) {
        ssize_t r = recv(s, buf + total, sizeof buf - total - 1, 0);
        if (r <= 0) break;
        total += (size_t)r;
        buf[total] = '\0';
        leituras++;

        if (declarado < 0) {
            char *fim = strstr(buf, "\r\n\r\n");
            if (fim) {
                cabecalho = (size_t)(fim - buf) + 4;
                char *cl = strstr(buf, "Content-Length: ");
                if (cl) declarado = atoi(cl + 16);
            }
        }
        if (declarado >= 0) {
            corpo_lido = total - cabecalho;
            if (corpo_lido >= (size_t)declarado) break;   /* a resposta acabou */
        }
    }
    printf("cliente com Content-Length: %d bytes de corpo em %d recv, saiu do laco\n",
           (int)corpo_lido, leituras);
    close(s);

    /* --- cliente 2: le ate o fim da conexao --- */
    s = conectar();
    pedir(s);
    struct timeval limite = { 0, 400000 };     /* para o exemplo terminar */
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &limite, sizeof limite);

    total = 0; leituras = 0;
    int pendurou = 0;
    for (;;) {
        ssize_t r = recv(s, buf, sizeof buf - 1, 0);
        if (r > 0) { total += (size_t)r; leituras++; continue; }
        if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) { pendurou = 1; break; }
        break;                                  /* r == 0: EOF de verdade */
    }
    printf("cliente que le ate EOF:     %d bytes em %d recv, e %s\n",
           (int)total, leituras,
           pendurou ? "PENDUROU esperando um fechamento que nao veio"
                    : "terminou por EOF");
    close(s);

    pthread_join(t, NULL);
    printf("\nOs dois receberam a resposta inteira. O segundo nao soube disso,\n"
           "porque a informacao de onde a mensagem termina vem do PROTOCOLO e\n"
           "nao do transporte - e keep-alive e o padrao do HTTP/1.1.\n");
    return 0;
}
