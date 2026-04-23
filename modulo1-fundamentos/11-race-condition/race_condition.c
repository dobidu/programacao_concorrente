/*
 * race_condition.c — Demonstração de race condition
 *
 * Conceitos: acesso não sincronizado, resultado não-determinístico
 * Execute múltiplas vezes e observe resultados diferentes!
 *
 * Teste com Helgrind: valgrind --tool=helgrind ./race_condition
 */
#include <pthread.h>
#include <stdio.h>

int contador = 0;  /* Variável GLOBAL compartilhada */

void* incrementar(void* arg) {
    (void)arg;
    for (int i = 0; i < 1000000; i++) {
        contador++;  /* NÃO é atômico! LOAD → ADD → STORE */
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, incrementar, NULL);
    pthread_create(&t2, NULL, incrementar, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    /* Esperado: 2000000 — obtido: valor MENOR e VARIÁVEL */
    printf("Contador: %d (esperado: 2000000)\n", contador);

    if (contador != 2000000)
        printf("⚠  Race condition detectada! Diferença: %d\n",
               2000000 - contador);

    return 0;
}
