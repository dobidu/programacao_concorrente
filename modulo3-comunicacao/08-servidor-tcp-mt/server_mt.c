/* server_mt.c — Servidor TCP multi-thread com signal handler */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8080
volatile sig_atomic_t running = 1;
int server_fd;

void handler(int sig) { (void)sig; running = 0; close(server_fd); }

void* tratar(void* arg) {
    int cfd = *((int*)arg); free(arg);
    char buf[1024]; ssize_t n;
    while ((n = recv(cfd, buf, sizeof(buf)-1, 0)) > 0) {
        buf[n] = '\0';
        printf("[%d] %s", cfd, buf);
        send(cfd, buf, n, 0);
    }
    printf("[%d] desconectou\n", cfd);
    close(cfd);
    return NULL;
}

int main() {
    signal(SIGINT, handler); signal(SIGPIPE, SIG_IGN);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt=1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {.sin_family=AF_INET, .sin_addr.s_addr=INADDR_ANY, .sin_port=htons(PORT)};
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("Servidor MT porta %d (Ctrl+C para sair)\n", PORT);
    while (running) {
        int *cfd = malloc(sizeof(int));
        *cfd = accept(server_fd, NULL, NULL);
        if (*cfd < 0) { free(cfd); continue; }
        printf("Novo cliente fd=%d\n", *cfd);
        pthread_t t; pthread_create(&t, NULL, tratar, cfd); pthread_detach(t);
    }
    printf("\nServidor encerrado.\n");
    return 0;
}
