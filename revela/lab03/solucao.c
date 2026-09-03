/* LAB-03 - SOLUÇÃO de referência: faixas em threads, e a primeira race.
 *
 * ATENÇÃO: este programa está ERRADO DE PROPÓSITO e não deve ser consertado
 * neste exercício. Ele existe para falhar de forma reproduzível.
 *
 * Cada thread processa uma faixa de linhas - e aí não há problema nenhum, porque
 * faixas não se sobrepõem. O defeito está no histograma: as N threads incrementam
 * o mesmo vetor `baldes` sem trava nenhuma.
 *
 * `baldes[l]++` não é uma operação. São três:
 *
 *     unsigned long v = baldes[l];   // LOAD
 *     v = v + 1;                     // ADD
 *     baldes[l] = v;                 // STORE
 *
 * Se duas threads fizerem LOAD do mesmo valor antes de qualquer STORE, um dos
 * dois incrementos desaparece. É a atualização perdida, e é por isso que o
 * campo hist= do resumo muda de execução para execução.
 *
 * Sem ThreadSanitizer no laboratório, a forma de torná-la reproduzível é abrir a
 * janela entre o LOAD e o STORE:
 *
 *     make amplifica ex03
 *     ./verifica.sh ./lab03/solucao entrada.ppm --threads 8
 */
#include "crono.h"
#include "filtro.h"
#include "janela.h"
#include "revela.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const Imagem *entrada;
    Imagem *saida;
    int y0, y1;
    unsigned long *baldes;      /* COMPARTILHADO entre todas as threads */
} Tarefa;

/* Luminância: a mesma conta de base/filtro.c, repetida aqui para que o LOAD e o
 * STORE do incremento fiquem visíveis no código do exercício. */
static inline unsigned char luminancia(const unsigned char *p) {
    unsigned v = (77u * p[0] + 150u * p[1] + 29u * p[2]) >> 8;
    return (unsigned char)(v > 255u ? 255u : v);
}

static void *trabalhar(void *arg) {
    Tarefa *t = arg;

    /* Esta parte está correta: faixas não se sobrepõem. */
    filtro_cinza(t->entrada, t->saida, t->y0, t->y1);

    /* Esta não. Três instruções, e nada impede que outra thread entre no meio. */
    for (int y = t->y0; y < t->y1; y++) {
        const unsigned char *le = t->saida->px + (size_t)y * t->saida->largura * 3;
        for (int x = 0; x < t->saida->largura; x++) {
            unsigned char l = luminancia(le + (size_t)x * 3);
            unsigned long v = t->baldes[l];     /* LOAD  */
            JANELA();                           /* a janela que o escalonador usa */
            t->baldes[l] = v + 1;               /* STORE */
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Imagem entrada, saida;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;

    unsigned long baldes[256];
    memset(baldes, 0, sizeof baldes);

    pthread_t *fios = calloc((size_t)o.threads, sizeof *fios);
    Tarefa *tarefas = calloc((size_t)o.threads, sizeof *tarefas);
    if (!fios || !tarefas) { perror("calloc"); return 1; }

    double t0 = agora_ms();
    int por_thread = (entrada.altura + o.threads - 1) / o.threads;
    for (int i = 0; i < o.threads; i++) {
        int y0 = i * por_thread;
        int y1 = y0 + por_thread;
        if (y1 > entrada.altura) y1 = entrada.altura;
        if (y0 > y1) y0 = y1;
        tarefas[i] = (Tarefa){ &entrada, &saida, y0, y1, baldes };
        if (pthread_create(&fios[i], NULL, trabalhar, &tarefas[i])) {
            perror("pthread_create"); return 1;
        }
    }
    for (int i = 0; i < o.threads; i++) pthread_join(fios[i], NULL);
    double ms = agora_ms() - t0;

    int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr, "tempo %.2f ms com %d thread(s)\n", ms, o.threads);

    free(fios); free(tarefas);
    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    return erro ? 1 : 0;
}
