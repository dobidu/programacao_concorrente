/* atomics.c — Operações atômicas C11 (sem lock) */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

atomic_int contador = 0;

void* incrementar(void* arg) {
    (void)arg;
    for (int i = 0; i < 1000000; i++)
        atomic_fetch_add(&contador, 1);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, incrementar, NULL);
    pthread_create(&t2, NULL, incrementar, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Contador atômico: %d (SEMPRE 2000000, sem lock)\n",
           atomic_load(&contador));
    return 0;
}
