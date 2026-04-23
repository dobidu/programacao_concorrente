/* shm_demo.c — Memória compartilhada POSIX entre processos */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

typedef struct { int valor; char msg[64]; } Dados;

int main() {
    const char *nome = "/lpii_shm";
    shm_unlink(nome);
    int fd = shm_open(nome, O_CREAT|O_RDWR, 0666);
    ftruncate(fd, sizeof(Dados));
    Dados *d = mmap(NULL, sizeof(Dados), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    d->valor = 0; strcpy(d->msg, "init");

    if (fork() == 0) {
        d->valor = 42;
        strcpy(d->msg, "Escrito pelo filho!");
        printf("Filho: valor=%d\n", d->valor);
        _exit(0);
    }
    wait(NULL);
    printf("Pai: valor=%d, msg='%s'\n", d->valor, d->msg);
    munmap(d, sizeof(Dados));
    shm_unlink(nome);
    return 0;
}
