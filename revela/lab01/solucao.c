/* LAB-01 - SOLUÇÃO de referência.
 *
 * Uma thread, nenhuma sincronização. O resultado deste programa é, por definição,
 * o correto - é contra ele que todas as versões concorrentes serão conferidas.
 */
#include "crono.h"
#include "revela.h"

#include <stdio.h>
#include <string.h>

static void cinza_faixa(const Imagem *entrada, Imagem *saida, int y0, int y1) {
    for (int y = y0; y < y1; y++) {
        const unsigned char *le = entrada->px + (size_t)y * entrada->largura * 3;
        unsigned char *es = saida->px + (size_t)y * saida->largura * 3;
        for (int x = 0; x < entrada->largura; x++) {
            const unsigned char *p = le + (size_t)x * 3;
            unsigned v = (77u * p[0] + 150u * p[1] + 29u * p[2]) >> 8;
            unsigned char c = (unsigned char)(v > 255u ? 255u : v);
            es[(size_t)x * 3 + 0] = c;
            es[(size_t)x * 3 + 1] = c;
            es[(size_t)x * 3 + 2] = c;
        }
    }
}

static void histograma(const Imagem *img, unsigned long baldes[256]) {
    for (int y = 0; y < img->altura; y++) {
        const unsigned char *le = img->px + (size_t)y * img->largura * 3;
        for (int x = 0; x < img->largura; x++)
            baldes[le[(size_t)x * 3]]++;   /* já em cinza: R basta */
    }
}

int main(int argc, char **argv) {
    Opcoes o;
    if (opcoes_ler(argc, argv, &o)) return 1;

    Imagem entrada, saida;
    if (ppm_ler(o.entrada, &entrada)) return 1;
    if (imagem_criar(&saida, entrada.largura, entrada.altura)) return 1;

    unsigned long baldes[256];
    double t0 = agora_ms();
    for (int r = 0; r < o.repeticoes; r++) {
        memset(baldes, 0, sizeof baldes);
        cinza_faixa(&entrada, &saida, 0, entrada.altura);
        histograma(&saida, baldes);
    }
    double ms = agora_ms() - t0;

    int erro = ppm_escrever(o.saida, &saida);
    imprimir_resumo(&o, baldes, (size_t)entrada.largura * entrada.altura);
    fprintf(stderr, "tempo %.2f ms (%d repetição(ões))\n", ms, o.repeticoes);

    imagem_destruir(&entrada);
    imagem_destruir(&saida);
    return erro ? 1 : 0;
}
