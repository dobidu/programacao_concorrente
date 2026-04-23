/* sem_named.c — Semáforo nomeado (entre processos independentes) */
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    sem_t *sem = sem_open("/lpii_demo", O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); return 1; }
    printf("PID %d: esperando semáforo...\n", getpid());
    sem_wait(sem);
    printf("PID %d: DENTRO da seção crítica (5s)\n", getpid());
    sleep(5);
    printf("PID %d: saindo\n", getpid());
    sem_post(sem);
    sem_close(sem);
    sem_unlink("/lpii_demo");
    return 0;
}
