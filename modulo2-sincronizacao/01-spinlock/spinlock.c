/* spinlock.c — Spinlock com atomic_flag (espera ocupada) */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_flag lock = ATOMIC_FLAG_INIT;
int contador = 0;

void acquire() { while (atomic_flag_test_and_set(&lock)); }
void release() { atomic_flag_clear(&lock); }

void* worker(void* arg) {
    (void)arg;
    for (int i = 0; i < 1000000; i++) {
        acquire();
        contador++;
        release();
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Contador: %d (esperado: 2000000)\n", contador);
    return 0;
}
