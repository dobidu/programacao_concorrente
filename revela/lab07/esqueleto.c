/* LAB-07 - Deadlock provocado e eliminado por ordenação.   preparatório, sem nota
 *
 * O Revela ganhou um filtro que precisa de duas regiões ao mesmo tempo: para
 * costurar a emenda entre dois blocos, é preciso travar o bloco de cima e o de
 * baixo. Duas threads fazem isso ao mesmo tempo, em ordens opostas:
 *
 *     thread A:  trava(bloco 0) ; trava(bloco 1)
 *     thread B:  trava(bloco 1) ; trava(bloco 0)
 *
 * Se A pegar a 0 e B pegar a 1 antes que qualquer uma peça a segunda, ninguém
 * solta e ninguém avança. É a espera circular - a quarta condição de Coffman - e
 * as outras três já estavam lá: exclusão mútua, posse e espera, e ausência de
 * preempção.
 *
 * O conserto NÃO é um terceiro mutex nem um trylock com retry: é impor uma ordem
 * global. Se toda thread trava sempre do índice menor para o maior, não existe
 * ciclo possível - o grafo de espera vira acíclico por construção.
 *
 *     make lab07                                        # ordenado: termina
 *     ./lab07/solucao entrada.ppm /tmp/s.ppm --modo ordenado --threads 8
 *     ./lab07/solucao entrada.ppm /tmp/s.ppm --modo cruzado  --threads 8   # trava
 *
 * Com o programa pendurado, em outro terminal:
 *     gdb -p $(pgrep -n -f lab07/solucao)
 *     (gdb) thread apply all bt
 * Duas threads em __lll_lock_wait, cada uma segurando o que a outra pede.
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER:
 *   1. o modo cruzado travando de forma reproduzível (use make amplifica);
 *   2. a saída de `thread apply all bt` mostrando o ciclo;
 *   3. o modo ordenado passando o portão;
 *   4. uma frase nomeando qual das quatro condições de Coffman a ordenação
 *      global quebra - e por que quebrar UMA basta.
 */
#include "crono.h"
#include "filtro.h"
#include "revela.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOCOS 2

typedef struct {
    const Imagem *entrada;
    Imagem *saida;
    pthread_mutex_t *travas;
    int primeiro, segundo;      /* ordem em que esta thread pega as travas */
    int y0, y1;
    unsigned long baldes[256];
} Tarefa;

static void *trabalhar(void *arg) {
    Tarefa *t = arg;

    pthread_mutex_lock(&t->travas[t->primeiro]);

    /* Entre pegar a primeira região e precisar da segunda, uma thread real faz
     * alguma coisa: mede a emenda, decide o raio do filtro, aloca. Aqui isso é
     * representado por uma pausa curta - e é ela que garante que TODAS as threads
     * segurem a primeira trava antes de qualquer uma pedir a segunda.
     *
     * Sem essa pausa o deadlock continua possível, mas raro: a primeira thread
     * costuma pegar as duas antes de a segunda começar. Um defeito que aparece
     * uma vez a cada mil execuções é pior de aprender do que um que aparece
     * sempre - e é exatamente o que torna concorrência difícil na prática. */
    usleep(2000);

    pthread_mutex_lock(&t->travas[t->segundo]);

    filtro_cinza(t->entrada, t->saida, t->y0, t->y1);
    histograma_faixa(t->saida, t->y0, t->y1, t->baldes);

    pthread_mutex_unlock(&t->travas[t->segundo]);
    pthread_mutex_unlock(&t->travas[t->primeiro]);
    return NULL;
}

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;
    int cruzado = !strcmp(o.modo, "cruzado");

    Imagem entrada, saida;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;

    pthread_mutex_t travas[BLOCOS];
    for (int i = 0; i < BLOCOS; i++) pthread_mutex_init(&travas[i], NULL);

    pthread_t *fios = calloc((size_t)o.threads, sizeof *fios);
    Tarefa *tarefas = calloc((size_t)o.threads, sizeof *tarefas);
    if (!fios || !tarefas) { perror("calloc"); return 1; }

    double t0 = agora_ms();
    int por_thread = (entrada.altura + o.threads - 1) / o.threads;
    for (int i = 0; i < o.threads; i++) {
        int y0 = i * por_thread, y1 = y0 + por_thread;
        if (y1 > entrada.altura) y1 = entrada.altura;
        if (y0 > y1) y0 = y1;

        /* ORDENADO: sempre do menor índice para o maior - sem ciclo possível.
         * CRUZADO:  metade das threads inverte - o ciclo aparece. */
        /* TODO: no modo cruzado, faça metade das threads pegar as travas na
         *       ordem inversa - é o que cria o ciclo. No modo ordenado, todas
         *       pegam na mesma ordem. Uma linha de cada. */
        int primeiro = 0, segundo = 1;
        (void)cruzado;

        tarefas[i] = (Tarefa){ &entrada, &saida, travas, primeiro, segundo, y0, y1, {0} };
        if (pthread_create(&fios[i], NULL, trabalhar, &tarefas[i])) {
            perror("pthread_create"); return 1;
        }
    }
    for (int i = 0; i < o.threads; i++) pthread_join(fios[i], NULL);
    double ms = agora_ms() - t0;

    unsigned long baldes[256];
    memset(baldes, 0, sizeof baldes);
    for (int i = 0; i < o.threads; i++)
        for (int k = 0; k < 256; k++) baldes[k] += tarefas[i].baldes[k];

    int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr, "ordem=%s threads=%d  %.2f ms\n",
            cruzado ? "cruzada" : "global", o.threads, ms);

    for (int i = 0; i < BLOCOS; i++) pthread_mutex_destroy(&travas[i]);
    free(fios); free(tarefas);
    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    return erro ? 1 : 0;
}
