/*
 * threads_basico.c — Criação e join de threads POSIX
 *
 * Conceitos: pthread_create, pthread_join, passagem de argumentos
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 4

void* tarefa(void* arg) {
    int id = *((int*)arg);
    printf("Thread %d em execução (tid=%lu)\n",
           id, (unsigned long)pthread_self());

    long soma = 0;
    for (long i = 0; i < 10000000L; i++)
        soma += i;

    printf("Thread %d terminou (soma=%ld)\n", id, soma);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i + 1;
        int ret = pthread_create(&threads[i], NULL, tarefa, &ids[i]);
        if (ret != 0) {
            fprintf(stderr, "Erro ao criar thread %d: %d\n", i, ret);
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Todas as %d threads terminaram.\n", NUM_THREADS);
    return 0;
}
