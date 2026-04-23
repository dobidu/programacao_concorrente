/*
 * MiniChat v2.0 — Servidor TCP multi-thread com broadcast
 * Conceitos: sockets TCP, thread por cliente, lista de clientes, broadcast
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 9000
#define MAX_CLIENTS 10
#define BUF_SIZE 512

/* Monitor: lista de clientes */
pthread_mutex_t clients_mtx = PTHREAD_MUTEX_INITIALIZER;
int clients[MAX_CLIENTS];
int n_clients = 0;

void add_client(int fd) {
    pthread_mutex_lock(&clients_mtx);
    if (n_clients < MAX_CLIENTS) clients[n_clients++] = fd;
    pthread_mutex_unlock(&clients_mtx);
}

void remove_client(int fd) {
    pthread_mutex_lock(&clients_mtx);
    for (int i = 0; i < n_clients; i++) {
        if (clients[i] == fd) {
            clients[i] = clients[--n_clients];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mtx);
}

void broadcast(const char* msg, int sender_fd) {
    pthread_mutex_lock(&clients_mtx);
    for (int i = 0; i < n_clients; i++) {
        if (clients[i] != sender_fd)
            send(clients[i], msg, strlen(msg), 0);
    }
    pthread_mutex_unlock(&clients_mtx);
}

void* handle_client(void* arg) {
    int fd = *((int*)arg); free(arg);
    add_client(fd);
    char buf[BUF_SIZE], msg[BUF_SIZE + 32];
    snprintf(msg, sizeof(msg), "[Servidor] Cliente %d entrou.\n", fd);
    broadcast(msg, fd);
    printf("%s", msg);

    ssize_t n;
    while ((n = recv(fd, buf, sizeof(buf)-1, 0)) > 0) {
        buf[n] = '\0';
        snprintf(msg, sizeof(msg), "[%d] %s", fd, buf);
        broadcast(msg, fd);
        printf("%s", msg);
    }

    snprintf(msg, sizeof(msg), "[Servidor] Cliente %d saiu.\n", fd);
    broadcast(msg, fd);
    printf("%s", msg);
    remove_client(fd);
    close(fd);
    return NULL;
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {.sin_family=AF_INET, .sin_addr.s_addr=INADDR_ANY, .sin_port=htons(PORT)};
    bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sfd, 5);
    printf("MiniChat v2.0 — Servidor TCP porta %d\n", PORT);
    printf("Conecte com: nc 127.0.0.1 %d\n\n", PORT);

    while (1) {
        int *cfd = malloc(sizeof(int));
        *cfd = accept(sfd, NULL, NULL);
        if (*cfd < 0) { free(cfd); continue; }
        pthread_t t; pthread_create(&t, NULL, handle_client, cfd); pthread_detach(t);
    }
}
