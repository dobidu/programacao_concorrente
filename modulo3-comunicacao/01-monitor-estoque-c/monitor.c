/* monitor.c — Monitor de controle de estoque (produtor-consumidor) */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX 5
typedef struct {
    int estoque, produzidos, consumidos;
    pthread_mutex_t mtx;
    pthread_cond_t nao_cheio, nao_vazio;
} EstoqueMonitor;

void init(EstoqueMonitor* m) {
    m->estoque = m->produzidos = m->consumidos = 0;
    pthread_mutex_init(&m->mtx, NULL);
    pthread_cond_init(&m->nao_cheio, NULL);
    pthread_cond_init(&m->nao_vazio, NULL);
}

void produzir(EstoqueMonitor* m, int id) {
    pthread_mutex_lock(&m->mtx);
    while (m->estoque >= MAX) pthread_cond_wait(&m->nao_cheio, &m->mtx);
    m->estoque++; m->produzidos++;
    printf("Produtor %d: estoque=%d\n", id, m->estoque);
    pthread_cond_signal(&m->nao_vazio);
    pthread_mutex_unlock(&m->mtx);
}

void consumir(EstoqueMonitor* m, int id) {
    pthread_mutex_lock(&m->mtx);
    while (m->estoque <= 0) pthread_cond_wait(&m->nao_vazio, &m->mtx);
    m->estoque--; m->consumidos++;
    printf("Consumidor %d: estoque=%d\n", id, m->estoque);
    pthread_cond_signal(&m->nao_cheio);
    pthread_mutex_unlock(&m->mtx);
}

EstoqueMonitor mon;

void* prod(void* a) { int id=*((int*)a); for(int i=0;i<10;i++){produzir(&mon,id);usleep(rand()%200000);} return NULL; }
void* cons(void* a) { int id=*((int*)a); for(int i=0;i<10;i++){consumir(&mon,id);usleep(rand()%300000);} return NULL; }

int main() {
    srand(42); init(&mon);
    pthread_t p[3], c[2]; int pid[]={1,2,3}, cid[]={1,2};
    for(int i=0;i<3;i++) pthread_create(&p[i],NULL,prod,&pid[i]);
    for(int i=0;i<2;i++) pthread_create(&c[i],NULL,cons,&cid[i]);
    for(int i=0;i<3;i++) pthread_join(p[i],NULL);
    for(int i=0;i<2;i++) pthread_join(c[i],NULL);
    printf("Produzidos: %d, Consumidos: %d\n", mon.produzidos, mon.consumidos);
    return 0;
}
