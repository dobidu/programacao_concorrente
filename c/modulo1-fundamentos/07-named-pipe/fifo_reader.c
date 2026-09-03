#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/tmp/lpii_fifo", O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    buf[n] = '\0';
    printf("Leitor recebeu: %s", buf);
    close(fd);
    return 0;
}
