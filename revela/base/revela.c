#include "revela.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void uso(const char *prog) {
    fprintf(stderr,
        "uso: %s <entrada.ppm> <saida.ppm> [--filtro cinza|desfoque]"
        " [--threads N] [--repeticoes N] [--modo NOME] [--clientes N]\n", prog);
}

int opcoes_ler(int argc, char **argv, Opcoes *o) {
    if (argc < 3) { uso(argv[0]); return -1; }
    o->entrada = argv[1];
    o->saida = argv[2];
    o->filtro = FILTRO_CINZA;
    o->threads = 1;
    o->repeticoes = 1;
    o->modo = "";
    o->clientes = 1;

    for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "--filtro") && i + 1 < argc) {
            i++;
            if (!strcmp(argv[i], "cinza")) o->filtro = FILTRO_CINZA;
            else if (!strcmp(argv[i], "desfoque")) o->filtro = FILTRO_DESFOQUE;
            else { fprintf(stderr, "filtro desconhecido: %s\n", argv[i]); return -1; }
        } else if (!strcmp(argv[i], "--threads") && i + 1 < argc) {
            o->threads = atoi(argv[++i]);
            if (o->threads < 1 || o->threads > 256) {
                fprintf(stderr, "--threads fora de 1..256\n"); return -1;
            }
        } else if (!strcmp(argv[i], "--clientes") && i + 1 < argc) {
            o->clientes = atoi(argv[++i]);
            if (o->clientes < 1 || o->clientes > 512) {
                fprintf(stderr, "--clientes fora de 1..512\n"); return -1;
            }
        } else if (!strcmp(argv[i], "--modo") && i + 1 < argc) {
            /* O oráculo aceita e ignora: ele precisa engolir a mesma linha de
             * comando do candidato para que a comparação seja possível. */
            o->modo = argv[++i];
        } else if (!strcmp(argv[i], "--repeticoes") && i + 1 < argc) {
            o->repeticoes = atoi(argv[++i]);
            if (o->repeticoes < 1) { fprintf(stderr, "--repeticoes < 1\n"); return -1; }
        } else {
            uso(argv[0]); return -1;
        }
    }
    return 0;
}

unsigned long histograma_checksum(const unsigned long baldes[256]) {
    /* FNV-1a sobre os 256 contadores. Qualquer incremento perdido muda o valor. */
    unsigned long h = 1469598103934665603UL;
    for (int i = 0; i < 256; i++) {
        unsigned long v = baldes[i];
        for (unsigned b = 0; b < sizeof v; b++) {
            h ^= (v >> (b * 8)) & 0xffUL;
            h *= 1099511628211UL;
        }
    }
    return h;
}

void imprimir_resumo(const Opcoes *o, const unsigned long baldes[256], size_t pixels) {
    size_t soma = 0;
    for (int i = 0; i < 256; i++) soma += baldes[i];
    printf("revela filtro=%s hist=0x%016lx baldes=%zu pixels=%zu\n",
           o->filtro == FILTRO_CINZA ? "cinza" : "desfoque",
           histograma_checksum(baldes), soma, pixels);
}
