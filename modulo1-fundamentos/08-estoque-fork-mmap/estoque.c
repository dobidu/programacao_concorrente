/*
 * estoque.c — Sistema de estoque com fork() + memória compartilhada (mmap)
 *
 * Conceitos: mmap MAP_SHARED|MAP_ANONYMOUS, race conditions entre processos
 * NOTA: Este exemplo NÃO usa sincronização — demonstra o PROBLEMA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>

typedef struct {
    int quantidade;
    int total_vendas;
    int total_reposicoes;
} Estoque;

int main() {
    Estoque *est = mmap(NULL, sizeof(Estoque),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    est->quantidade = 50;
    est->total_vendas = 0;
    est->total_reposicoes = 0;

    printf("Estoque inicial: %d unidades\n\n", est->quantidade);

    /* 3 processos vendedores */
    for (int i = 0; i < 3; i++) {
        if (fork() == 0) {
            srand(time(NULL) + getpid());
            for (int j = 0; j < 10; j++) {
                if (est->quantidade > 0) {
                    est->quantidade--;
                    est->total_vendas++;
                    printf("[Vendedor %d] Vendeu! Estoque: %d\n",
                           i + 1, est->quantidade);
                } else {
                    printf("[Vendedor %d] SEM ESTOQUE!\n", i + 1);
                }
                usleep(rand() % 200000);
            }
            _exit(0);
        }
    }

    /* 1 processo repositor */
    if (fork() == 0) {
        srand(time(NULL) + getpid());
        for (int j = 0; j < 5; j++) {
            usleep(500000);
            int qtd = 5 + rand() % 10;
            est->quantidade += qtd;
            est->total_reposicoes += qtd;
            printf("[Repositor] +%d unidades. Estoque: %d\n",
                   qtd, est->quantidade);
        }
        _exit(0);
    }

    for (int i = 0; i < 4; i++) wait(NULL);

    int esperado = 50 - est->total_vendas + est->total_reposicoes;
    printf("\n=== RELATÓRIO ===\n");
    printf("Estoque final:    %d\n", est->quantidade);
    printf("Total vendas:     %d\n", est->total_vendas);
    printf("Total reposições: %d\n", est->total_reposicoes);
    printf("Esperado:         %d\n", esperado);
    if (est->quantidade != esperado)
        printf("⚠  INCONSISTÊNCIA (race condition!)\n");

    munmap(est, sizeof(Estoque));
    return 0;
}
