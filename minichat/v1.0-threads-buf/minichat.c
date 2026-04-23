/*
 * MiniChat v1.0 — Thread por usuário, buffer compartilhado
 * Conceitos: pthread_create, memória compartilhada entre threads
 * NOTA: SEM mutex — race conditions possíveis!
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_MSGS 100
#define MAX_USERS 4

char mensagens[MAX_MSGS][256];
int msg_count = 0;

void* usuario(void* arg) {
    int id = *((int*)arg);
    int meu_idx = 0;
    for (int i = 0; i < 5; i++) {
        /* Enviar mensagem */
        snprintf(mensagens[msg_count], 256, "User%d: Msg %d", id, i);
        msg_count++;  /* Race condition aqui! */
        usleep(100000 + (rand() % 200000));

        /* Ler novas mensagens */
        while (meu_idx < msg_count) {
            printf("[User%d vê] %s\n", id, mensagens[meu_idx]);
            meu_idx++;
        }
    }
    return NULL;
}

int main() {
    printf("MiniChat v1.0 — %d usuários (SEM sincronização!)\n", MAX_USERS);
    pthread_t threads[MAX_USERS];
    int ids[MAX_USERS];
    for (int i = 0; i < MAX_USERS; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, usuario, &ids[i]);
    }
    for (int i = 0; i < MAX_USERS; i++)
        pthread_join(threads[i], NULL);
    printf("Total mensagens: %d (podem ter sido perdidas!)\n", msg_count);
    return 0;
}
