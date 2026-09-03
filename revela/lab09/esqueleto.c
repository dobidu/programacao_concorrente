/* LAB-09 - A fila vira monitor.   preparatório, sem nota
 *
 * O LAB-06 funciona, mas a trava está exposta: quem usa a fila precisa saber que
 * existe um mutex, quais condições esperar e em que ordem. Isso não escala para
 * um sistema com mais de um autor.
 *
 * Um monitor é o dado mais a sincronização, atrás de uma interface que não vaza
 * nem uma coisa nem outra. Aqui:
 *
 *     fila_por / fila_tirar / fila_encerrar     é tudo o que o cliente vê
 *     pthread_mutex_t, pthread_cond_t           não aparecem fora deste arquivo
 *
 * O invariante do monitor, que é a parte que se declara e se mantém:
 *
 *     0 <= quantos <= CAPACIDADE
 *     e todo caminho que sai de uma função pública deixa a trava liberada
 *     e acorda quem a mudança de estado tornou elegível a prosseguir.
 *
 * O teste do exercício é mecânico: `grep pthread_mutex lab09/solucao.c` só pode
 * casar dentro da implementação do monitor, nunca no código que o usa.
 *
 *   make lab09 && ./verifica.sh ./lab09/esqueleto entrada.ppm --threads 8
 *   grep -n pthread_mutex lab09/esqueleto.c     # só dentro do monitor
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER: o portão passando, o grep limpo fora do monitor, e o invariante
 * escrito em comentário - em português, não em código.
 */
#include "crono.h"
#include "filtro.h"
#include "revela.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAPACIDADE 8            /* pequena de propósito: o produtor VAI esperar */
#define LINHAS_POR_BLOCO 16

typedef struct { int y0, y1; } Bloco;

/* ── O MONITOR ──────────────────────────────────────────────────────────
 * Daqui até fila_encerrar() é a implementação. Nada abaixo desta região
 * pode mencionar mutex ou variável de condição. */

typedef struct {
    Bloco itens[CAPACIDADE];
    int inicio, quantos;
    int fim;                                /* produtor não manda mais nada */
    pthread_mutex_t trava;
    pthread_cond_t nao_cheio, nao_vazio;
    unsigned long esperas_produtor, esperas_consumidor;
} Fila;

static void fila_iniciar(Fila *f) {
    f->inicio = f->quantos = f->fim = 0;
    f->esperas_produtor = f->esperas_consumidor = 0;
    pthread_mutex_init(&f->trava, NULL);
    pthread_cond_init(&f->nao_cheio, NULL);
    pthread_cond_init(&f->nao_vazio, NULL);
}

static void fila_destruir(Fila *f) {
    pthread_mutex_destroy(&f->trava);
    pthread_cond_destroy(&f->nao_cheio);
    pthread_cond_destroy(&f->nao_vazio);
}

static void fila_por(Fila *f, Bloco b) {
    pthread_mutex_lock(&f->trava);
    while (f->quantos == CAPACIDADE) {          /* LAÇO, não if */
        f->esperas_produtor++;
        pthread_cond_wait(&f->nao_cheio, &f->trava);
    }
    f->itens[(f->inicio + f->quantos) % CAPACIDADE] = b;
    f->quantos++;
    pthread_cond_signal(&f->nao_vazio);
    pthread_mutex_unlock(&f->trava);
}

/* Devolve 1 se pegou um bloco, 0 se acabou o trabalho e não virá mais.
 *
 * TODO: esta é a operação do monitor que falta. Ela precisa, nesta ordem:
 *   · tomar a trava;
 *   · esperar enquanto não houver bloco E o fim não tiver sido marcado;
 *   · devolver 0, com a trava liberada, se a fila esvaziou e não virá mais nada;
 *   · senão, retirar o bloco mais antigo, avisar quem espera por espaço, liberar
 *     a trava e devolver 1.
 *
 * Todo caminho de saída libera a trava. Um `return` no meio, sem unlock, trava o
 * programa inteiro - e o portão vai acusar como travamento, não como divergência.
 */
static int fila_tirar(Fila *f, Bloco *saida) {
    (void)f; (void)saida;
    return 0;
}

static void fila_encerrar(Fila *f) {
    pthread_mutex_lock(&f->trava);
    f->fim = 1;
    pthread_cond_broadcast(&f->nao_vazio);      /* broadcast: acorda TODOS */
    pthread_mutex_unlock(&f->trava);
}

/* ── FIM DO MONITOR ─────────────────────────────────────────────────── */

typedef struct {
    Fila *fila;
    const Imagem *entrada;
    Imagem *saida;
    unsigned long baldes[256];                  /* privado: nada a sincronizar */
} Consumidor;

static void *consumir(void *arg) {
    Consumidor *c = arg;
    Bloco b;
    while (fila_tirar(c->fila, &b)) {
        filtro_cinza(c->entrada, c->saida, b.y0, b.y1);
        histograma_faixa(c->saida, b.y0, b.y1, c->baldes);
    }
    return NULL;
}

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Imagem entrada, saida;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;

    Fila fila;
    fila_iniciar(&fila);

    pthread_t *fios = calloc((size_t)o.threads, sizeof *fios);
    Consumidor *cons = calloc((size_t)o.threads, sizeof *cons);
    if (!fios || !cons) { perror("calloc"); return 1; }

    double t0 = agora_ms();
    for (int i = 0; i < o.threads; i++) {
        cons[i].fila = &fila;
        cons[i].entrada = &entrada;
        cons[i].saida = &saida;
        if (pthread_create(&fios[i], NULL, consumir, &cons[i])) {
            perror("pthread_create"); return 1;
        }
    }

    for (int y = 0; y < entrada.altura; y += LINHAS_POR_BLOCO) {
        int y1 = y + LINHAS_POR_BLOCO;
        if (y1 > entrada.altura) y1 = entrada.altura;
        fila_por(&fila, (Bloco){ y, y1 });
    }
    fila_encerrar(&fila);

    for (int i = 0; i < o.threads; i++) pthread_join(fios[i], NULL);
    double ms = agora_ms() - t0;

    unsigned long baldes[256];
    memset(baldes, 0, sizeof baldes);
    for (int i = 0; i < o.threads; i++)
        for (int k = 0; k < 256; k++) baldes[k] += cons[i].baldes[k];

    int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr, "threads=%d capacidade=%d  %.2f ms  "
                    "produtor esperou %lu vez(es), consumidores %lu\n",
            o.threads, CAPACIDADE, ms,
            fila.esperas_produtor, fila.esperas_consumidor);

    free(fios); free(cons);
    fila_destruir(&fila);
    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    return erro ? 1 : 0;
}
