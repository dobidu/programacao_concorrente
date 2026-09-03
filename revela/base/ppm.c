#include "ppm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pula espaços em branco e comentários (# até o fim da linha) do cabeçalho. */
static int pular_lixo(FILE *f) {
    int c;
    for (;;) {
        c = fgetc(f);
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            continue;
        } else {
            return c;
        }
    }
}

static int ler_inteiro(FILE *f, int *saida) {
    int c = pular_lixo(f);
    if (c < '0' || c > '9') return -1;
    long v = 0;
    while (c >= '0' && c <= '9') {
        v = v * 10 + (c - '0');
        if (v > 100000) return -1;          /* limite são para material didático */
        c = fgetc(f);
    }
    *saida = (int)v;
    return 0;
}

int imagem_criar(Imagem *img, int largura, int altura) {
    if (largura <= 0 || altura <= 0) return -1;
    img->largura = largura;
    img->altura = altura;
    img->px = malloc((size_t)largura * (size_t)altura * 3u);
    if (!img->px) { perror("malloc"); return -1; }
    return 0;
}

void imagem_destruir(Imagem *img) {
    free(img->px);
    img->px = NULL;
    img->largura = img->altura = 0;
}

int ppm_ler(const char *caminho, Imagem *img) {
    FILE *f = fopen(caminho, "rb");
    if (!f) { perror(caminho); return -1; }

    int c1 = pular_lixo(f), c2 = fgetc(f);
    if (c1 != 'P' || c2 != '6') {
        fprintf(stderr, "%s: não é PPM binário (P6)\n", caminho);
        fclose(f);
        return -1;
    }

    int largura = 0, altura = 0, maximo = 0;
    if (ler_inteiro(f, &largura) || ler_inteiro(f, &altura) || ler_inteiro(f, &maximo)) {
        fprintf(stderr, "%s: cabeçalho inválido\n", caminho);
        fclose(f);
        return -1;
    }
    if (maximo != 255) {
        fprintf(stderr, "%s: só 8 bits por canal (maxval 255), veio %d\n", caminho, maximo);
        fclose(f);
        return -1;
    }

    if (imagem_criar(img, largura, altura)) { fclose(f); return -1; }

    size_t n = imagem_bytes(img);
    if (fread(img->px, 1, n, f) != n) {
        fprintf(stderr, "%s: dados de pixel incompletos\n", caminho);
        imagem_destruir(img);
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

int ppm_escrever(const char *caminho, const Imagem *img) {
    FILE *f = fopen(caminho, "wb");
    if (!f) { perror(caminho); return -1; }
    fprintf(f, "P6\n%d %d\n255\n", img->largura, img->altura);
    size_t n = imagem_bytes(img);
    if (fwrite(img->px, 1, n, f) != n) {
        perror(caminho);
        fclose(f);
        return -1;
    }
    if (fclose(f)) { perror(caminho); return -1; }
    return 0;
}
