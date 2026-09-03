#include "filtro.h"

/* Luminância BT.601 em ponto fixo: (77R + 150G + 29B) >> 8.
 * Inteiro de propósito - em ponto flutuante o resultado poderia depender da
 * ordem das operações, e o oráculo sequencial deixaria de ser oráculo. */
static inline unsigned char luminancia(const unsigned char *p) {
    unsigned v = (77u * p[0] + 150u * p[1] + 29u * p[2]) >> 8;
    return (unsigned char)(v > 255u ? 255u : v);
}

void filtro_cinza(const Imagem *entrada, Imagem *saida, int y0, int y1) {
    for (int y = y0; y < y1; y++) {
        const unsigned char *le = entrada->px + (size_t)y * entrada->largura * 3;
        unsigned char *es = saida->px + (size_t)y * saida->largura * 3;
        for (int x = 0; x < entrada->largura; x++) {
            unsigned char c = luminancia(le + (size_t)x * 3);
            es[(size_t)x * 3 + 0] = c;
            es[(size_t)x * 3 + 1] = c;
            es[(size_t)x * 3 + 2] = c;
        }
    }
}

/* Núcleo 1x5 [1 4 6 4 1] / 16 - soma 16, então a divisão é um deslocamento. */
static const unsigned PESO[5] = {1u, 4u, 6u, 4u, 1u};

static inline int limitar(int v, int menor, int maior) {
    return v < menor ? menor : (v > maior ? maior : v);
}

void filtro_desfoque_h(const Imagem *entrada, Imagem *saida, int y0, int y1) {
    for (int y = y0; y < y1; y++) {
        const unsigned char *le = entrada->px + (size_t)y * entrada->largura * 3;
        unsigned char *es = saida->px + (size_t)y * saida->largura * 3;
        for (int x = 0; x < entrada->largura; x++) {
            for (int canal = 0; canal < 3; canal++) {
                unsigned soma = 0;
                for (int k = -2; k <= 2; k++) {
                    int xv = limitar(x + k, 0, entrada->largura - 1);
                    soma += PESO[k + 2] * le[(size_t)xv * 3 + canal];
                }
                es[(size_t)x * 3 + canal] = (unsigned char)(soma >> 4);
            }
        }
    }
}

void filtro_desfoque_v(const Imagem *entrada, Imagem *saida, int y0, int y1) {
    for (int y = y0; y < y1; y++) {
        unsigned char *es = saida->px + (size_t)y * saida->largura * 3;
        for (int x = 0; x < entrada->largura; x++) {
            for (int canal = 0; canal < 3; canal++) {
                unsigned soma = 0;
                for (int k = -2; k <= 2; k++) {
                    int yv = limitar(y + k, 0, entrada->altura - 1);
                    const unsigned char *le =
                        entrada->px + (size_t)yv * entrada->largura * 3;
                    soma += PESO[k + 2] * le[(size_t)x * 3 + canal];
                }
                es[(size_t)x * 3 + canal] = (unsigned char)(soma >> 4);
            }
        }
    }
}

void histograma_faixa(const Imagem *img, int y0, int y1, unsigned long baldes[256]) {
    for (int y = y0; y < y1; y++) {
        const unsigned char *le = img->px + (size_t)y * img->largura * 3;
        for (int x = 0; x < img->largura; x++)
            baldes[luminancia(le + (size_t)x * 3)]++;
    }
}
