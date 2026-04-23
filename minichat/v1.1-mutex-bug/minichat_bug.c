/*
 * MiniChat v1.1 (BUG) — Mutex aplicado PARCIALMENTE
 * BUG: protege msg_count++ mas NÃO a escrita em mensagens[]
 * Execute com Helgrind para detectar: valgrind --tool=helgrind ./minichat_bug
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_MSGS 100
#define MAX_USERS 4

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
char mensagens[MAX_MSGS][256];
int msg_count = 0;

void* usuario(void* arg) {
    int id = *((int*)arg);
    for (int i = 0; i < 5; i++) {
        /* BUG: escreve no buffer FORA do lock */
        snprintf(mensagens[msg_count], 256, "User%d: Msg %d", id, i);

        pthread_mutex_lock(&mtx);
        msg_count++;  /* Só isto está protegido */
        pthread_mutex_unlock(&mtx);

        usleep(100000);
    }
    return NULL;
}

int main() {
    printf("MiniChat v1.1 (BUG) — Mutex parcial!\n");
    pthread_t t[MAX_USERS]; int ids[MAX_USERS];
    for (int i=0; i<MAX_USERS; i++) { ids[i]=i+1; pthread_create(&t[i],NULL,usuario,&ids[i]); }
    for (int i=0; i<MAX_USERS; i++) pthread_join(t[i],NULL);
    printf("Mensagens (podem estar corrompidas):\n");
    for (int i=0; i<msg_count; i++) printf("  [%d] %s\n", i, mensagens[i]);
    return 0;
}
