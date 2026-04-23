/* client_tcp.c — Cliente TCP básico */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {.sin_family=AF_INET, .sin_port=htons(PORT)};
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    const char *msg = "Ola do cliente!";
    send(sock, msg, strlen(msg), 0);
    char buf[1024]; int n = read(sock, buf, sizeof(buf)-1); buf[n]='\0';
    printf("Resposta: %s", buf);
    close(sock);
    return 0;
}
