/* trylock.c — Padrão try-lock com trabalho alternativo */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t recurso = PTHREAD_MUTEX_INITIALIZER;
int valor_recurso = 0;

void* worker(void* arg) {
    int id = *((int*)arg);
    int hits = 0, misses = 0;
    for (int i = 0; i < 20; i++) {
        if (pthread_mutex_trylock(&recurso) == 0) {
            valor_recurso++;
            printf("Thread %d: recurso=%d\n", id, valor_recurso);
            usleep(50000);
            pthread_mutex_unlock(&recurso);
            hits++;
        } else {
            printf("Thread %d: ocupado, fazendo outra tarefa\n", id);
            usleep(30000);
            misses++;
        }
    }
    printf("Thread %d: %d hits, %d misses\n", id, hits, misses);
    return NULL;
}

int main() {
    pthread_t t1, t2, t3;
    int ids[] = {1, 2, 3};
    pthread_create(&t1, NULL, worker, &ids[0]);
    pthread_create(&t2, NULL, worker, &ids[1]);
    pthread_create(&t3, NULL, worker, &ids[2]);
    pthread_join(t1, NULL); pthread_join(t2, NULL); pthread_join(t3, NULL);
    printf("Valor final: %d\n", valor_recurso);
    pthread_mutex_destroy(&recurso);
    return 0;
}
