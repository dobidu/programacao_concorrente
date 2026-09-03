/* pthread_spin.c — Spinlock POSIX (pthread_spinlock_t) */
#include <pthread.h>
#include <stdio.h>

pthread_spinlock_t spin;
int contador = 0;

void* worker(void* arg) {
    (void)arg;
    for (int i = 0; i < 1000000; i++) {
        pthread_spin_lock(&spin);
        contador++;
        pthread_spin_unlock(&spin);
    }
    return NULL;
}

int main() {
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Contador: %d (esperado: 2000000)\n", contador);
    pthread_spin_destroy(&spin);
    return 0;
}
