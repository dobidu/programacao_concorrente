/* filosofos.c — Jantar dos Filósofos (solução: quebra de simetria) */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 5
pthread_mutex_t garfo[N];

void* filosofo(void* arg) {
    int id = *((int*)arg);
    int esq = id, dir = (id + 1) % N;

    for (int i = 0; i < 3; i++) {
        printf("Filósofo %d pensando...\n", id);
        usleep(rand() % 500000);

        /* Quebrar simetria: filósofo 0 pega na ordem inversa */
        if (id == 0) {
            pthread_mutex_lock(&garfo[dir]);
            pthread_mutex_lock(&garfo[esq]);
        } else {
            pthread_mutex_lock(&garfo[esq]);
            pthread_mutex_lock(&garfo[dir]);
        }

        printf("Filósofo %d COMENDO (garfos %d e %d)\n", id, esq, dir);
        usleep(rand() % 300000);

        pthread_mutex_unlock(&garfo[esq]);
        pthread_mutex_unlock(&garfo[dir]);
    }
    printf("Filósofo %d terminou.\n", id);
    return NULL;
}

int main() {
    srand(42);
    pthread_t threads[N];
    int ids[N];
    for (int i = 0; i < N; i++) pthread_mutex_init(&garfo[i], NULL);
    for (int i = 0; i < N; i++) { ids[i]=i; pthread_create(&threads[i], NULL, filosofo, &ids[i]); }
    for (int i = 0; i < N; i++) pthread_join(threads[i], NULL);
    for (int i = 0; i < N; i++) pthread_mutex_destroy(&garfo[i]);
    printf("Todos os filósofos comeram sem deadlock!\n");
    return 0;
}
