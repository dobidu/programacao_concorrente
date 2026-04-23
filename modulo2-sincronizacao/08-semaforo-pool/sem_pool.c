/* sem_pool.c — Pool de conexões com semáforo de contagem */
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_CONN 3
#define NUM_CLIENTS 8
sem_t pool;

void* cliente(void* arg) {
    int id = *((int*)arg);
    printf("Cliente %d: tentando conexão...\n", id);
    sem_wait(&pool);
    printf("Cliente %d: CONECTADO\n", id);
    sleep(2);
    printf("Cliente %d: liberando\n", id);
    sem_post(&pool);
    return NULL;
}

int main() {
    sem_init(&pool, 0, MAX_CONN);
    pthread_t threads[NUM_CLIENTS];
    int ids[NUM_CLIENTS];
    for (int i = 0; i < NUM_CLIENTS; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, cliente, &ids[i]);
    }
    for (int i = 0; i < NUM_CLIENTS; i++)
        pthread_join(threads[i], NULL);
    sem_destroy(&pool);
    printf("Todos os clientes atendidos.\n");
    return 0;
}
