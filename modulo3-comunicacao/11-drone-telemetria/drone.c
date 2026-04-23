/* drone.c — Simulação de telemetria de drone via UDP */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#define PORT 9090

typedef struct {
    int drone_id;
    float lat, lon, alt, vel;
    time_t ts;
} Telemetria;

int main(int argc, char *argv[]) {
    int id = (argc > 1) ? atoi(argv[1]) : 1;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sa = {.sin_family=AF_INET,.sin_port=htons(PORT)};
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    srand(time(NULL) + id);
    printf("Drone %d: enviando telemetria...\n", id);
    for (int i = 0; i < 10; i++) {
        Telemetria t = {id, -7.12f + (float)rand()/RAND_MAX*0.01f,
            -34.86f + (float)rand()/RAND_MAX*0.01f,
            100.0f + rand()%50, 10.0f + rand()%20, time(NULL)};
        sendto(fd, &t, sizeof(t), 0, (struct sockaddr*)&sa, sizeof(sa));
        printf("  Drone %d: lat=%.4f lon=%.4f alt=%.0f\n", id, t.lat, t.lon, t.alt);
        usleep(500000);
    }
    close(fd); return 0;
}
