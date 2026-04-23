/* ticket_lock.c — Algoritmo do Ticket Lock (FIFO justo) */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

#define NUM_USERS 5
atomic_int next_ticket = 0;
atomic_int now_serving = 0;

void acquire() {
    int my = atomic_fetch_add(&next_ticket, 1);
    while (atomic_load(&now_serving) != my);
}
void release() { atomic_fetch_add(&now_serving, 1); }

void* usar_recurso(void* arg) {
    int id = *((int*)arg);
    acquire();
    printf("Usuário %d: acessando recurso (ticket=%d)\n", id, now_serving);
    usleep(100000);
    printf("Usuário %d: liberando\n", id);
    release();
    return NULL;
}

int main() {
    pthread_t threads[NUM_USERS];
    int ids[NUM_USERS];
    for (int i = 0; i < NUM_USERS; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, usar_recurso, &ids[i]);
    }
    for (int i = 0; i < NUM_USERS; i++)
        pthread_join(threads[i], NULL);
    printf("Todos atendidos em ordem FIFO.\n");
    return 0;
}
