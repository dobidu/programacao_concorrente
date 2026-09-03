/* barreira_cv.c — Barreira reutilizável com mutex + condvar */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define N 4
pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bcond = PTHREAD_COND_INITIALIZER;
int bcount = 0, bround = 0;

void barreira_wait(int total) {
    pthread_mutex_lock(&bmtx);
    int my_round = bround;
    bcount++;
    if (bcount == total) {
        bcount = 0;
        bround++;
        pthread_cond_broadcast(&bcond);
    } else {
        while (bround == my_round)
            pthread_cond_wait(&bcond, &bmtx);
    }
    pthread_mutex_unlock(&bmtx);
}

void* worker(void* arg) {
    int id = *((int*)arg);
    for (int fase = 1; fase <= 3; fase++) {
        printf("Thread %d: fase %d\n", id, fase);
        usleep(id * 200000);
        barreira_wait(N);
    }
    return NULL;
}

int main() {
    pthread_t threads[N]; int ids[N];
    for (int i = 0; i < N; i++) { ids[i]=i+1; pthread_create(&threads[i], NULL, worker, &ids[i]); }
    for (int i = 0; i < N; i++) pthread_join(threads[i], NULL);
    printf("Todas as 3 fases completadas.\n");
    return 0;
}
