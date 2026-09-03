/* LAB-04 - False sharing: medir, alinhar, medir de novo.   preparatório, sem nota
 *
 * O LAB-03 mostrou que um contador compartilhado se corrompe. A resposta certa é
 * dar um acumulador a cada thread e somar no fim - e isso está correto: o
 * resultado bate com o oráculo, sempre, sem trava nenhuma.
 *
 * Mas há duas formas de arrumar esses acumuladores na memória:
 *
 *   CONTÍGUO   unsigned long soma[N];        8 bytes por thread, lado a lado
 *   ALINHADO   struct { unsigned long v;
 *                       char _[56]; }        um por linha de cache
 *
 * No primeiro, oito acumuladores ocupam 64 bytes - exatamente UMA linha de cache.
 * As threads não compartilham dado nenhum: cada uma escreve só na sua posição. Mas
 * o hardware não trafega bytes, trafega linhas. Cada escrita de qualquer thread
 * invalida a linha nos outros sete núcleos, que precisam buscá-la de novo. É o
 * false sharing: contenção sem compartilhamento.
 *
 * No segundo, cada acumulador tem a sua linha e o ping-pong desaparece. Mesmo
 * resultado, mesmo número de instruções, tempo muito diferente.
 *
 *     make lab04 && ./lab04/esqueleto entrada.ppm saida.ppm --threads 8 --repeticoes 40
 *
 * Como vem, os dois arranjos são iguais e o ganho dá 1,00x. Complete o tipo
 * alinhado e meça de novo.
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER: os dois tempos e uma frase explicando a diferença pela linha de cache.
 * Rode também com --threads 1: o ganho some. Entender por que some vale tanto
 * quanto entender por que ele existe com 8.
 */
#include "crono.h"
#include "filtro.h"
#include "revela.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINHA_CACHE 64

/* Um acumulador por thread, vizinhos na memória. */
typedef struct { unsigned long v; } Acumulador;

/* TODO: faça deste tipo um acumulador que ocupe uma linha de cache inteira.
 *       São duas coisas, e as duas são necessárias:
 *         · preenchimento até LINHA_CACHE bytes, para que dois acumuladores
 *           não caibam na mesma linha;
 *         · __attribute__((aligned(LINHA_CACHE))), para que o primeiro comece
 *           numa fronteira de linha - sem isso o vetor inteiro fica deslocado.
 *       Como está, é idêntico ao contíguo e o ganho dá 1,00x. */
typedef struct {
    unsigned long v;
} AcumuladorAlinhado;

typedef struct {
    const Imagem *img;
    int y0, y1;
    int passadas;                      /* trabalho dentro da thread, não fora */
    volatile unsigned long *destino;   /* ver a nota sobre volatile abaixo */
} Tarefa;

/* Soma a luminância da faixa, uma escrita à memória por pixel.
 *
 * Por que `volatile`: sem ele, o gcc a -O2 percebe que ninguém mais escreve
 * naquele endereço, mantém a soma num registrador durante todo o laço e grava
 * uma única vez no fim. Sem escrita repetida não há invalidação de linha, e o
 * false sharing simplesmente não acontece - a medição daria 1,00x e a conclusão
 * seria errada.
 *
 * Isto não é truque para inflar o resultado: é o que faz a medição medir o que
 * se quer medir. Num programa real a escrita repetida existe porque o valor é
 * de fato consultado, atualizado ou publicado entre as iterações. */
static void *trabalhar(void *arg) {
    Tarefa *t = arg;
    for (int p = 0; p < t->passadas; p++)
        for (int y = t->y0; y < t->y1; y++) {
            const unsigned char *le = t->img->px + (size_t)y * t->img->largura * 3;
            for (int x = 0; x < t->img->largura; x++)
                *t->destino += le[(size_t)x * 3];
        }
    return NULL;
}

static double medir(const Imagem *img, int threads, int repeticoes,
                    volatile unsigned long *(*destino_de)(int),
                    unsigned long *total) {
    pthread_t *fios = calloc((size_t)threads, sizeof *fios);
    Tarefa *tarefas = calloc((size_t)threads, sizeof *tarefas);
    if (!fios || !tarefas) { perror("calloc"); exit(1); }

    int por_thread = (img->altura + threads - 1) / threads;

    /* As threads são criadas UMA vez e repetem o trabalho por dentro. Criá-las a
     * cada repetição faria pthread_create dominar a medição - com 8 threads e 30
     * repetições são 240 criações, e o que se mediria seria o escalonador, não a
     * linha de cache. */
    for (int i = 0; i < threads; i++) *destino_de(i) = 0;

    double t0 = agora_ms();
    for (int i = 0; i < threads; i++) {
        int y0 = i * por_thread, y1 = y0 + por_thread;
        if (y1 > img->altura) y1 = img->altura;
        if (y0 > y1) y0 = y1;
        tarefas[i] = (Tarefa){ img, y0, y1, repeticoes, destino_de(i) };
        if (pthread_create(&fios[i], NULL, trabalhar, &tarefas[i])) {
            perror("pthread_create"); exit(1);
        }
    }
    for (int i = 0; i < threads; i++) pthread_join(fios[i], NULL);
    double ms = agora_ms() - t0;

    *total = 0;
    for (int i = 0; i < threads; i++) *total += *destino_de(i);

    free(fios); free(tarefas);
    return ms;
}

static Acumulador *g_contiguo;
static AcumuladorAlinhado *g_alinhado;
static volatile unsigned long *contiguo_de(int i) { return &g_contiguo[i].v; }
static volatile unsigned long *alinhado_de(int i) { return &g_alinhado[i].v; }

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Imagem entrada, saida;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;
    filtro_cinza(&entrada, &saida, 0, entrada.altura);

    g_contiguo = calloc((size_t)o.threads, sizeof *g_contiguo);
    if (!g_contiguo) { perror("calloc"); return 1; }
    if (posix_memalign((void **)&g_alinhado, LINHA_CACHE,
                       (size_t)o.threads * sizeof *g_alinhado)) {
        perror("posix_memalign"); return 1;
    }
    memset(g_alinhado, 0, (size_t)o.threads * sizeof *g_alinhado);

    unsigned long soma_cont = 0, soma_alin = 0;
    double ms_cont = medir(&saida, o.threads, o.repeticoes, contiguo_de, &soma_cont);
    double ms_alin = medir(&saida, o.threads, o.repeticoes, alinhado_de, &soma_alin);

    if (soma_cont != soma_alin) {
        fprintf(stderr, "ERRO: os dois arranjos deram somas diferentes\n");
        return 1;
    }

    unsigned long baldes[256];
    memset(baldes, 0, sizeof baldes);
    histograma_faixa(&saida, 0, saida.altura, baldes);

    int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr,
        "threads=%d repeticoes=%d soma=%lu\n"
        "  acumuladores contíguos (%zu B cada, %d cabem numa linha)  %8.2f ms\n"
        "  acumuladores alinhados (%zu B cada, um por linha)         %8.2f ms\n"
        "  o alinhamento é %.2fx mais rápido\n",
        o.threads, o.repeticoes, soma_alin,
        sizeof(Acumulador), (int)(LINHA_CACHE / sizeof(Acumulador)), ms_cont,
        sizeof(AcumuladorAlinhado), ms_alin,
        ms_alin > 0 ? ms_cont / ms_alin : 0.0);

    free(g_contiguo); free(g_alinhado);
    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    return erro ? 1 : 0;
}
