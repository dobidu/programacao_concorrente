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
    if (fd < 0) { perror("open"); return 1; }

    const char *msg = "Mensagem via FIFO!\n";
    /* Conferir o retorno de write nao e zelo: um write curto e legitimo, e
       ignora-lo perde bytes em silencio. O compilador cobra isso com
       -Wunused-result, que -Wall ja liga. */
    if (write(fd, msg, strlen(msg)) < 0) { perror("write"); close(fd); return 1; }
    printf("Escritor: mensagem enviada.\n");
    close(fd);
    unlink(fifo);
    return 0;
}
