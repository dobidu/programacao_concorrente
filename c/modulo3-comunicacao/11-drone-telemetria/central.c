/* central.c — Central de controle recebe telemetria UDP */
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#define PORT 9090

typedef struct {
    int drone_id;
    float lat, lon, alt, vel;
    time_t ts;
} Telemetria;

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sa = {.sin_family=AF_INET,.sin_addr.s_addr=INADDR_ANY,.sin_port=htons(PORT)};
    bind(fd, (struct sockaddr*)&sa, sizeof(sa));
    printf("Central aguardando telemetria na porta %d...\n", PORT);
    for (int i = 0; i < 30; i++) {
        Telemetria t;
        recvfrom(fd, &t, sizeof(t), 0, NULL, NULL);
        printf("[Drone %d] lat=%.4f lon=%.4f alt=%.0fm vel=%.0fkm/h\n",
               t.drone_id, t.lat, t.lon, t.alt, t.vel);
    }
    close(fd); return 0;
}
