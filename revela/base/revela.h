/* revela.h - interface comum a todas as versões do Revela.
 *
 * Todas as versões - sequencial, em processos, em threads, em rede - aceitam a
 * mesma linha de comando e imprimem o mesmo resumo. É isso que permite comparar
 * qualquer uma com o oráculo sequencial usando só `cmp` e `diff`.
 */
#ifndef REVELA_H
#define REVELA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ppm.h"

typedef enum { FILTRO_CINZA, FILTRO_DESFOQUE } Filtro;

typedef struct {
    const char *entrada;
    const char *saida;
    Filtro filtro;
    int threads;        /* ignorado pelas versões sequenciais */
    int repeticoes;     /* repete o processamento, para medir sem ruído de I/O */
    const char *modo;   /* variante escolhida pelo exercício; o oráculo ignora */
    int clientes;       /* exercícios de rede; o oráculo ignora */
} Opcoes;

/* Devolve 0 em sucesso. Em erro imprime o uso e devolve -1. */
int opcoes_ler(int argc, char **argv, Opcoes *o);

/* Resumo determinístico impresso por toda versão, em stdout:
 *     revela filtro=cinza threads=4 hist=0x... pixels=960000
 * O checksum do histograma é o que torna uma race detectável sem ferramenta:
 * ele muda quando um incremento se perde. */
void imprimir_resumo(const Opcoes *o, const unsigned long baldes[256], size_t pixels);

/* Soma de verificação do histograma. Sensível a incremento perdido. */
unsigned long histograma_checksum(const unsigned long baldes[256]);

#ifdef __cplusplus
}
#endif

#endif
