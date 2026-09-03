/* gerar_imagem.c - produz a imagem de entrada do Revela, sem depender de arquivo
 * binário versionado. Determinística: a mesma entrada em qualquer máquina.
 *
 *     ./gerar_imagem entrada.ppm 1200 800
 */
#include "ppm.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "uso: %s <saida.ppm> <largura> <altura>\n", argv[0]);
        return 1;
    }
    int largura = atoi(argv[2]), altura = atoi(argv[3]);

    Imagem img;
    if (imagem_criar(&img, largura, altura)) return 1;

    /* Gradientes cruzados mais um xadrez: dá luminância bem espalhada (o
     * histograma fica interessante) e fronteiras nítidas, onde uma faixa
     * processada errado salta aos olhos. */
    for (int y = 0; y < altura; y++) {
        unsigned char *linha = img.px + (size_t)y * largura * 3;
        for (int x = 0; x < largura; x++) {
            int quadro = ((x / 32) + (y / 32)) & 1;
            linha[(size_t)x * 3 + 0] = (unsigned char)((x * 255) / (largura - 1));
            linha[(size_t)x * 3 + 1] = (unsigned char)((y * 255) / (altura - 1));
            linha[(size_t)x * 3 + 2] = (unsigned char)(quadro ? 40 : 215);
        }
    }

    int r = ppm_escrever(argv[1], &img);
    imagem_destruir(&img);
    return r ? 1 : 0;
}
