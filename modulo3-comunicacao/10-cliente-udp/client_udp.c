/* client_udp.c — Cliente UDP */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sa = {.sin_family=AF_INET,.sin_port=htons(PORT)};
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
    const char *msg = "Ola UDP!";
    sendto(fd, msg, strlen(msg), 0, (struct sockaddr*)&sa, sizeof(sa));
    char buf[1024]; int n = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
    buf[n]='\0'; printf("Resposta: %s\n", buf);
    close(fd); return 0;
}
