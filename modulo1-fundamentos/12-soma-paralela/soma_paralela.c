/*
 * soma_paralela.c — Soma paralela de array com threads (sem race condition)
 *
 * Conceitos: scatter-gather, divisão de trabalho, resultados parciais
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ARRAY_SIZE 10000000
#define NUM_THREADS 4

double array[ARRAY_SIZE];
double resultados[NUM_THREADS];

typedef struct {
    int id, inicio, fim;
} ThreadArgs;

void* somar_parcial(void* arg) {
    ThreadArgs* a = (ThreadArgs*)arg;
    double soma = 0.0;
    for (int i = a->inicio; i < a->fim; i++)
        soma += array[i];
    resultados[a->id] = soma;  /* Posição EXCLUSIVA: sem race! */
    return NULL;
}

int main() {
    srand(42);
    for (int i = 0; i < ARRAY_SIZE; i++)
        array[i] = (double)rand() / RAND_MAX;

    /* ── Versão sequencial ── */
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    double soma_seq = 0;
    for (int i = 0; i < ARRAY_SIZE; i++)
        soma_seq += array[i];
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_seq = (t1.tv_sec-t0.tv_sec)*1000.0 + (t1.tv_nsec-t0.tv_nsec)/1e6;

    /* ── Versão paralela ── */
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int chunk = ARRAY_SIZE / NUM_THREADS;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = (ThreadArgs){i, i*chunk,
            (i == NUM_THREADS-1) ? ARRAY_SIZE : (i+1)*chunk};
        pthread_create(&threads[i], NULL, somar_parcial, &args[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    double soma_par = 0;
    for (int i = 0; i < NUM_THREADS; i++)
        soma_par += resultados[i];
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_par = (t1.tv_sec-t0.tv_sec)*1000.0 + (t1.tv_nsec-t0.tv_nsec)/1e6;

    printf("Sequencial: soma=%.2f  tempo=%.2f ms\n", soma_seq, ms_seq);
    printf("Paralelo:   soma=%.2f  tempo=%.2f ms  (%d threads)\n",
           soma_par, ms_par, NUM_THREADS);
    printf("Speedup:    %.2fx\n", ms_seq / ms_par);

    return 0;
}
