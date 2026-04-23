#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

int main() {
    const char *fifo = "/tmp/lpii_fifo";
    mkfifo(fifo, 0666);
    printf("Escritor: esperando leitor...\n");
    int fd = open(fifo, O_WRONLY);
    const char *msg = "Mensagem via FIFO!\n";
    write(fd, msg, strlen(msg));
    printf("Escritor: mensagem enviada.\n");
    close(fd);
    unlink(fifo);
    return 0;
}
