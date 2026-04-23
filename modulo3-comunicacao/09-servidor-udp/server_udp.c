/* server_udp.c — Servidor UDP */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sa = {.sin_family=AF_INET,.sin_addr.s_addr=INADDR_ANY,.sin_port=htons(PORT)};
    bind(fd, (struct sockaddr*)&sa, sizeof(sa));
    printf("UDP porta %d...\n", PORT);
    struct sockaddr_in cli; socklen_t len = sizeof(cli);
    char buf[1024];
    int n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&cli, &len);
    buf[n] = '\0';
    printf("Recebido: %s\n", buf);
    const char *resp = "Datagrama OK!";
    sendto(fd, resp, strlen(resp), 0, (struct sockaddr*)&cli, len);
    close(fd);
    return 0;
}
