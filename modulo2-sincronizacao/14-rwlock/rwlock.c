/* rwlock.c — Readers-Writers Lock POSIX */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
int dados = 0;

void* leitor(void* arg) {
    int id = *((int*)arg);
    for (int i = 0; i < 5; i++) {
        pthread_rwlock_rdlock(&rwlock);
        printf("Leitor %d leu: %d\n", id, dados);
        pthread_rwlock_unlock(&rwlock);
        usleep(100000);
    }
    return NULL;
}

void* escritor(void* arg) {
    int id = *((int*)arg);
    for (int i = 0; i < 3; i++) {
        pthread_rwlock_wrlock(&rwlock);
        dados++;
        printf("Escritor %d escreveu: %d\n", id, dados);
        pthread_rwlock_unlock(&rwlock);
        usleep(300000);
    }
    return NULL;
}

int main() {
    pthread_t leitores[5], escritores[2];
    int lid[5], eid[2];
    for (int i = 0; i < 5; i++) { lid[i]=i+1; pthread_create(&leitores[i], NULL, leitor, &lid[i]); }
    for (int i = 0; i < 2; i++) { eid[i]=i+1; pthread_create(&escritores[i], NULL, escritor, &eid[i]); }
    for (int i = 0; i < 5; i++) pthread_join(leitores[i], NULL);
    for (int i = 0; i < 2; i++) pthread_join(escritores[i], NULL);
    pthread_rwlock_destroy(&rwlock);
    printf("Final: dados = %d\n", dados);
    return 0;
}
