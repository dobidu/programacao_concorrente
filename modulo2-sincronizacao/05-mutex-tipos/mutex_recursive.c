/* mutex_recursive.c — Mutex recursivo e errorcheck */
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

int main() {
    /* ── Mutex RECURSIVE ── */
    pthread_mutex_t rmutex;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&rmutex, &attr);
    pthread_mutexattr_destroy(&attr);

    printf("=== Mutex Recursive ===\n");
    pthread_mutex_lock(&rmutex);
    printf("Lock 1 OK\n");
    pthread_mutex_lock(&rmutex);   /* OK com recursive! */
    printf("Lock 2 OK (recursive)\n");
    pthread_mutex_unlock(&rmutex);
    pthread_mutex_unlock(&rmutex);
    printf("Ambos unlocks OK\n\n");
    pthread_mutex_destroy(&rmutex);

    /* ── Mutex ERRORCHECK ── */
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_t emutex;
    pthread_mutex_init(&emutex, &attr);
    pthread_mutexattr_destroy(&attr);

    printf("=== Mutex Errorcheck ===\n");
    pthread_mutex_lock(&emutex);
    printf("Lock 1 OK\n");
    int ret = pthread_mutex_lock(&emutex);
    if (ret == EDEADLK)
        printf("Lock 2 detectou EDEADLK (deadlock evitado!)\n");
    pthread_mutex_unlock(&emutex);
    pthread_mutex_destroy(&emutex);

    return 0;
}
