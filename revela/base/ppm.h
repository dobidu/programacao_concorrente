/* ppm.h - leitura e escrita de imagem PPM (P6, 8 bits por canal).
 *
 * Dado pronto. O tempo da disciplina é para decidir sincronização, não para
 * analisar cabeçalho de arquivo.
 */
#ifndef REVELA_PPM_H
#define REVELA_PPM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct {
    int largura;
    int altura;
    unsigned char *px;   /* largura*altura*3 bytes, RGB entrelaçado */
} Imagem;

/* Devolve 0 em sucesso. Em erro, escreve o motivo em stderr e devolve -1. */
int  ppm_ler(const char *caminho, Imagem *img);
int  ppm_escrever(const char *caminho, const Imagem *img);

int  imagem_criar(Imagem *img, int largura, int altura);
void imagem_destruir(Imagem *img);

/* Bytes totais do buffer de pixels. */
static inline size_t imagem_bytes(const Imagem *img) {
    return (size_t)img->largura * (size_t)img->altura * 3u;
}

#ifdef __cplusplus
}
#endif

#endif
