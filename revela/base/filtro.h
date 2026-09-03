/* filtro.h - os filtros do Revela, todos puros sobre faixas de linhas.
 *
 * Todo filtro processa o intervalo de linhas [y0, y1) e escreve só nele. É essa
 * propriedade que permite dividir a imagem entre threads sem sincronização
 * nenhuma no caminho dos pixels - e é ela que o estudante quebra, de propósito,
 * no exercício do histograma.
 */
#ifndef REVELA_FILTRO_H
#define REVELA_FILTRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ppm.h"

/* Luminância BT.601 em inteiros: mesma conta em toda máquina, sem ponto
 * flutuante e portanto sem divergência de arredondamento entre execuções. */
void filtro_cinza(const Imagem *entrada, Imagem *saida, int y0, int y1);

/* Desfoque separável 1x5. Duas passagens: horizontal e depois vertical.
 * A segunda passagem lê linhas que a primeira escreveu - por isso as fases
 * precisam de barreira, e é aí que mora o exercício do latch. */
void filtro_desfoque_h(const Imagem *entrada, Imagem *saida, int y0, int y1);
void filtro_desfoque_v(const Imagem *entrada, Imagem *saida, int y0, int y1);

/* Histograma de luminância: 256 baldes. Escreve num vetor compartilhado - é o
 * ponto onde a divisão em faixas deixa de ser suficiente. */
void histograma_faixa(const Imagem *img, int y0, int y1, unsigned long baldes[256]);

#ifdef __cplusplus
}
#endif

#endif
