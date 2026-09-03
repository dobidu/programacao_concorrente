/* LAB-01 - O oráculo: filtro sequencial e tempo de base.   preparatório, sem nota
 *
 * Tudo o que envolve arquivo já está pronto em base/. O que falta é o miolo do
 * filtro: converter cada pixel RGB em cinza.
 *
 * Use luminância BT.601 em INTEIROS:  (77*R + 150*G + 29*B) >> 8
 * Inteiro não é capricho: em ponto flutuante o resultado poderia depender da
 * ordem das operações, e aí o oráculo deixaria de ser oráculo.
 *
 *   make lab01 && ./verifica.sh ./lab01/esqueleto entrada.ppm --filtro cinza
 *
 * O QUE VOCÊ TEM DE CONSEGUIR FAZER: o portão passando, mais o tempo impresso em stderr.
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
            (void)le; (void)es;
            /* TODO: calcule a luminância do pixel x e escreva-a nos três canais
             *       de saída. O pixel de entrada começa em le + x*3. */
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
