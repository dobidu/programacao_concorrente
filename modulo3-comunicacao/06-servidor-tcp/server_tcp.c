/* server_tcp.c — Servidor TCP básico */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080

int main() {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {.sin_family=AF_INET, .sin_addr.s_addr=INADDR_ANY, .sin_port=htons(PORT)};
    bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sfd, 5);
    printf("Servidor TCP porta %d...\n", PORT);
    int cfd = accept(sfd, NULL, NULL);
    char buf[1024]; int n = read(cfd, buf, sizeof(buf)-1); buf[n]='\0';
    printf("Recebido: %s\n", buf);
    const char *resp = "OK do servidor!\n";
    send(cfd, resp, strlen(resp), 0);
    close(cfd); close(sfd);
    return 0;
}
