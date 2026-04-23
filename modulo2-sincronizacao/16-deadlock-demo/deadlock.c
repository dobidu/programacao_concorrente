/* deadlock.c — Demonstração de deadlock + correção
 * Compile e execute: observe o travamento.
 * Depois compile com -DCORRETO para ver a solução. */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void* thread_A(void* arg) {
    (void)arg;
#ifdef CORRETO
    pthread_mutex_lock(&m1); pthread_mutex_lock(&m2);  /* Ordem: m1→m2 */
#else
    pthread_mutex_lock(&m1); sleep(1); pthread_mutex_lock(&m2);
#endif
    printf("Thread A: adquiriu ambos\n");
    pthread_mutex_unlock(&m2); pthread_mutex_unlock(&m1);
    return NULL;
}

void* thread_B(void* arg) {
    (void)arg;
#ifdef CORRETO
    pthread_mutex_lock(&m1); pthread_mutex_lock(&m2);  /* MESMA ordem */
#else
    pthread_mutex_lock(&m2); sleep(1); pthread_mutex_lock(&m1);  /* DEADLOCK! */
#endif
    printf("Thread B: adquiriu ambos\n");
    pthread_mutex_unlock(&m1); pthread_mutex_unlock(&m2);
    return NULL;
}

int main() {
#ifdef CORRETO
    printf("=== VERSÃO CORRIGIDA (mesma ordem de locks) ===\n");
#else
    printf("=== VERSÃO COM DEADLOCK (ordens opostas) ===\n");
    printf("Se travar, pressione Ctrl+C\n");
#endif
    pthread_t a, b;
    pthread_create(&a, NULL, thread_A, NULL);
    pthread_create(&b, NULL, thread_B, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("Ambas as threads terminaram.\n");
    return 0;
}
