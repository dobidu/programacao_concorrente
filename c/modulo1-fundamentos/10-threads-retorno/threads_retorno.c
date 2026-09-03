#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void* calcular_fatorial(void* arg) {
    int n = *((int*)arg);
    long* resultado = malloc(sizeof(long));
    *resultado = 1;
    for (int i = 2; i <= n; i++)
        *resultado *= i;
    return (void*)resultado;
}

int main() {
    int valores[] = {5, 8, 10, 12};
    pthread_t threads[4];
    for (int i = 0; i < 4; i++)
        pthread_create(&threads[i], NULL, calcular_fatorial, &valores[i]);
    for (int i = 0; i < 4; i++) {
        void* ret;
        pthread_join(threads[i], &ret);
        long* res = (long*)ret;
        printf("Fatorial de %d = %ld\n", valores[i], *res);
        free(res);
    }
    return 0;
}
