/* revela_seq.c - o oráculo.
 *
 * Uma thread, um processo, nenhuma sincronização. O que este programa escreve é,
 * por definição, o resultado correto: toda versão concorrente do Revela é
 * conferida contra ele, byte a byte.
 *
 * Não é conveniência de correção. É a definição operacional de corretude para um
 * programa cuja saída não pode depender do escalonador.
 */
#include "crono.h"
#include "filtro.h"
#include "revela.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Imagem entrada, saida, temp;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;
    if (imagem_criar(&temp, entrada.largura, entrada.altura)) return 1;

    unsigned long baldes[256];
    double t0 = agora_ms();

    for (int r = 0; r < o.repeticoes; r++) {
        memset(baldes, 0, sizeof baldes);
        if (o.filtro == FILTRO_CINZA) {
            filtro_cinza(&entrada, &saida, 0, entrada.altura);
        } else {
            filtro_desfoque_h(&entrada, &temp, 0, entrada.altura);
            filtro_desfoque_v(&temp, &saida, 0, entrada.altura);
        }
        histograma_faixa(&saida, 0, saida.altura, baldes);
    }

    double ms = agora_ms() - t0;

    int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr, "tempo %.2f ms (%d repetição(ões))\n", ms, o.repeticoes);

    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    imagem_destruir(&temp);
    return erro ? 1 : 0;
}
