/*
 * MiniChat v1.1 (CORRIGIDO) — Mutex protege TODA a seção crítica
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
        pthread_mutex_lock(&mtx);
        /* Toda a operação é atômica agora */
        snprintf(mensagens[msg_count], 256, "User%d: Msg %d", id, i);
        msg_count++;
        pthread_mutex_unlock(&mtx);
        usleep(100000);
    }
    return NULL;
}

int main() {
    printf("MiniChat v1.1 (CORRIGIDO) — Mutex completo\n");
    pthread_t t[MAX_USERS]; int ids[MAX_USERS];
    for (int i=0; i<MAX_USERS; i++) { ids[i]=i+1; pthread_create(&t[i],NULL,usuario,&ids[i]); }
    for (int i=0; i<MAX_USERS; i++) pthread_join(t[i],NULL);
    printf("Mensagens (todas corretas):\n");
    for (int i=0; i<msg_count; i++) printf("  [%d] %s\n", i, mensagens[i]);
    return 0;
}
