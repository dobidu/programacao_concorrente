/* prod_cons.c — Produtor-Consumidor com variáveis de condição */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define BUF_SIZE 5
int buffer[BUF_SIZE];
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nao_cheio = PTHREAD_COND_INITIALIZER;
pthread_cond_t nao_vazio = PTHREAD_COND_INITIALIZER;

void* produtor(void* arg) {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);
        while (count == BUF_SIZE)
            pthread_cond_wait(&nao_cheio, &mutex);
        buffer[count++] = i;
        printf("Produzido: %d (buf: %d/%d)\n", i, count, BUF_SIZE);
        pthread_cond_signal(&nao_vazio);
        pthread_mutex_unlock(&mutex);
        usleep(100000);
    }
    return NULL;
}

void* consumidor(void* arg) {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);
        while (count == 0)
            pthread_cond_wait(&nao_vazio, &mutex);
        int item = buffer[--count];
        printf("Consumido: %d (buf: %d/%d)\n", item, count, BUF_SIZE);
        pthread_cond_signal(&nao_cheio);
        pthread_mutex_unlock(&mutex);
        usleep(150000);
    }
    return NULL;
}

int main() {
    pthread_t prod, cons;
    pthread_create(&prod, NULL, produtor, NULL);
    pthread_create(&cons, NULL, consumidor, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    return 0;
}
