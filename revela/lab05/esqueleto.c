/* LAB-05 - Três consertos para o mesmo contador, medidos.   preparatório, sem nota
 *
 * O LAB-03 mostrou o histograma perdendo incrementos. Há três respostas, e todas
 * corrigem. Elas diferem no custo, e a diferença é o conteúdo do exercício.
 *
 *   --modo mutex     um pthread_mutex_t protege o vetor inteiro.
 *                    Correto e simples. Toda thread disputa a mesma trava a cada
 *                    pixel: a seção crítica é minúscula e a contenção é máxima.
 *
 *   --modo atomico   __atomic_fetch_add sobre cada balde.
 *                    Sem trava e sem bloqueio; o hardware garante o
 *                    read-modify-write. Ainda há contenção de linha de cache
 *                    quando duas threads batem no mesmo balde.
 *
 *   --modo reducao   cada thread tem o seu vetor; soma-se tudo no fim.
 *                    Nenhuma sincronização no caminho quente. É o padrão
 *                    scatter-gather: privatizar, trabalhar, reduzir.
 *
 *   make lab05
 *   for m in mutex atomico reducao; do
 *       ./verifica.sh ./lab05/solucao entrada.ppm --threads 8 --modo $m
 *   done
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER: o portão passando nos três modos e os três tempos. E a pergunta que
 * vale a nota: por que a redução ganha, se ela faz exatamente o mesmo número de
 * incrementos que as outras duas?
 */
#include "crono.h"
#include "filtro.h"
#include "janela.h"
#include "revela.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { MODO_MUTEX, MODO_ATOMICO, MODO_REDUCAO } Modo;

typedef struct {
    const Imagem *entrada;
    Imagem *saida;
    int y0, y1;
    Modo modo;
    unsigned long *compartilhado;   /* mutex e atomico escrevem aqui */
    unsigned long *privado;         /* reducao escreve aqui */
    pthread_mutex_t *trava;
} Tarefa;

static inline unsigned char luminancia(const unsigned char *p) {
    unsigned v = (77u * p[0] + 150u * p[1] + 29u * p[2]) >> 8;
    return (unsigned char)(v > 255u ? 255u : v);
}

static void *trabalhar(void *arg) {
    Tarefa *t = arg;
    filtro_cinza(t->entrada, t->saida, t->y0, t->y1);

    for (int y = t->y0; y < t->y1; y++) {
        const unsigned char *le = t->saida->px + (size_t)y * t->saida->largura * 3;
        for (int x = 0; x < t->saida->largura; x++) {
            unsigned char l = luminancia(le + (size_t)x * 3);
            switch (t->modo) {
            case MODO_MUTEX:
                /* TODO: proteja o incremento abaixo com t->trava. Deixe a
                 *       JANELA() onde está: com a trava certa, ela deixa de
                 *       importar, e é isso que o portão vai provar. */
                { unsigned long v = t->compartilhado[l];
                  JANELA();
                  t->compartilhado[l] = v + 1; }
                break;
            case MODO_ATOMICO:
                /* TODO: um incremento atômico sobre t->compartilhado[l].
                 *       Veja __atomic_fetch_add e o memory order relaxado - aqui
                 *       não há dado publicado junto, só uma contagem. */
                break;
            case MODO_REDUCAO:
                /* TODO: incremente o vetor privado desta thread. */
                (void)l;
                break;
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Modo modo;
    if (!strcmp(o.modo, "mutex")) modo = MODO_MUTEX;
    else if (!strcmp(o.modo, "atomico")) modo = MODO_ATOMICO;
    else if (!strcmp(o.modo, "reducao") || !*o.modo) modo = MODO_REDUCAO;
    else { fprintf(stderr, "modo desconhecido: %s\n", o.modo); return 1; }

    Imagem entrada, saida;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;

    unsigned long baldes[256];
    memset(baldes, 0, sizeof baldes);
    pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;

    pthread_t *fios = calloc((size_t)o.threads, sizeof *fios);
    Tarefa *tarefas = calloc((size_t)o.threads, sizeof *tarefas);
    unsigned long *privados = calloc((size_t)o.threads * 256, sizeof *privados);
    if (!fios || !tarefas || !privados) { perror("calloc"); return 1; }

    double t0 = agora_ms();
    int por_thread = (entrada.altura + o.threads - 1) / o.threads;
    for (int i = 0; i < o.threads; i++) {
        int y0 = i * por_thread, y1 = y0 + por_thread;
        if (y1 > entrada.altura) y1 = entrada.altura;
        if (y0 > y1) y0 = y1;
        tarefas[i] = (Tarefa){ &entrada, &saida, y0, y1, modo,
                               baldes, privados + (size_t)i * 256, &trava };
        if (pthread_create(&fios[i], NULL, trabalhar, &tarefas[i])) {
            perror("pthread_create"); return 1;
        }
    }
    for (int i = 0; i < o.threads; i++) pthread_join(fios[i], NULL);

    /* TODO: no modo reducao, some aqui os vetores privados em baldes[]. */

    double ms = agora_ms() - t0;

    int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr, "modo=%s threads=%d  %.2f ms\n",
            modo == MODO_MUTEX ? "mutex" : modo == MODO_ATOMICO ? "atomico" : "reducao",
            o.threads, ms);

    free(fios); free(tarefas); free(privados);
    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    return erro ? 1 : 0;
}
