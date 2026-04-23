/* timedwait.c — Consumidor com timeout usando pthread_cond_timedwait */
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define BUF 5
int buffer[BUF], count = 0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nv = PTHREAD_COND_INITIALIZER;

void* produtor(void* arg) {
    for (int i = 0; i < 3; i++) {
        sleep(3);  /* Produção lenta */
        pthread_mutex_lock(&mtx);
        buffer[count++] = i;
        printf("Produzido: %d\n", i);
        pthread_cond_signal(&nv);
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

void* consumidor(void* arg) {
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mtx);
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 2;  /* Timeout: 2 segundos */

        while (count == 0) {
            int ret = pthread_cond_timedwait(&nv, &mtx, &ts);
            if (ret == ETIMEDOUT) {
                printf("Consumidor: TIMEOUT (2s sem dados)\n");
                pthread_mutex_unlock(&mtx);
                goto next;
            }
        }
        printf("Consumido: %d\n", buffer[--count]);
        pthread_mutex_unlock(&mtx);
        next: ;
    }
    return NULL;
}

int main() {
    pthread_t p, c;
    pthread_create(&p, NULL, produtor, NULL);
    pthread_create(&c, NULL, consumidor, NULL);
    pthread_join(p, NULL); pthread_join(c, NULL);
    return 0;
}
