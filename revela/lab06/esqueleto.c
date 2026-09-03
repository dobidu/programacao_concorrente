/* LAB-06 - Fila de blocos com backpressure.   preparatório, sem nota
 *
 * A divisão estática em faixas iguais assume que toda faixa custa o mesmo. Não
 * assume mais: aqui o trabalho vai para uma fila limitada, e cada consumidor pega
 * o próximo bloco quando termina o anterior. Faixa cara não trava as outras.
 *
 * A fila é limitada de propósito. Se fosse ilimitada, um produtor rápido comeria a
 * memória toda antes de o primeiro consumidor acordar. Cheia, ele espera - é o
 * backpressure, e é a razão de existirem DUAS variáveis de condição:
 *
 *     nao_cheio  o produtor espera aqui quando não há espaço
 *     nao_vazio  o consumidor espera aqui quando não há trabalho
 *
 * Três coisas que o exercício cobra:
 *   · esperar em LAÇO (while, não if): um wait pode acordar sem que a condição
 *     valha - despertar espúrio, e também o caso de outro consumidor ter pegado
 *     o bloco antes;
 *   · encerrar sem espera ocupada: o produtor marca fim e faz broadcast;
 *   · nenhum consumidor pode ficar dormindo depois do fim.
 *
 *   make lab06 && ./verifica.sh ./lab06/esqueleto entrada.ppm --threads 8
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER: o portão passando, mais a tabela de esperas com --threads 1, 2 e 8. O
 * mesmo programa troca de gargalo: com um consumidor o produtor espera dezenas de
 * vezes (a fila vive cheia), com oito quem espera é o consumidor (a fila vive
 * vazia). Diga qual das duas situações é a saudável, e por quê.
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
    /* TODO: espere enquanto a fila estiver cheia. Use while, não if, e conte a
     *       espera em f->esperas_produtor. */
    f->itens[(f->inicio + f->quantos) % CAPACIDADE] = b;
    f->quantos++;
    pthread_cond_signal(&f->nao_vazio);
    pthread_mutex_unlock(&f->trava);
}

/* Devolve 1 se pegou um bloco, 0 se acabou o trabalho e não virá mais. */
static int fila_tirar(Fila *f, Bloco *saida) {
    pthread_mutex_lock(&f->trava);
    /* TODO: espere enquanto não houver bloco E o produtor não tiver marcado fim.
     *       Pense em qual das duas condições solta a espera. */
    if (f->quantos == 0) {                      /* fila vazia E fim marcado */
        pthread_mutex_unlock(&f->trava);
        return 0;
    }
    *saida = f->itens[f->inicio];
    f->inicio = (f->inicio + 1) % CAPACIDADE;
    f->quantos--;
    pthread_cond_signal(&f->nao_cheio);
    pthread_mutex_unlock(&f->trava);
    return 1;
}

static void fila_encerrar(Fila *f) {
    pthread_mutex_lock(&f->trava);
    /* TODO: marque o fim e acorde quem está dormindo. signal ou broadcast?
     *       Escolha e saiba dizer por quê - a resposta errada deixa consumidor
     *       dormindo para sempre, e o portão vai acusar como travamento. */
    pthread_mutex_unlock(&f->trava);
}

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
