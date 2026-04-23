/* barreira.c — Sincronização de fases com pthread_barrier */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define N 4
pthread_barrier_t barreira;

void* worker(void* arg) {
    int id = *((int*)arg);
    printf("Thread %d: fase 1 (trabalho %ds)...\n", id, id);
    sleep(id);
    printf("Thread %d: chegou na barreira\n", id);
    int ret = pthread_barrier_wait(&barreira);
    if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
        printf("Thread %d: [SERIAL] consolidando resultados\n", id);
    printf("Thread %d: fase 2 iniciada\n", id);
    return NULL;
}

int main() {
    pthread_barrier_init(&barreira, NULL, N);
    pthread_t threads[N];
    int ids[N];
    for (int i = 0; i < N; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }
    for (int i = 0; i < N; i++)
        pthread_join(threads[i], NULL);
    pthread_barrier_destroy(&barreira);
    return 0;
}
