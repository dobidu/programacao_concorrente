CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
all: deadlock deadlock_fix
deadlock: deadlock.c
	$(CC) $(CFLAGS) -o deadlock deadlock.c
deadlock_fix: deadlock.c
	$(CC) $(CFLAGS) -DCORRETO -o deadlock_fix deadlock.c
clean:
	rm -f deadlock deadlock_fix
run_bug: deadlock
	@echo "Pode travar! Use Ctrl+C para sair."
	timeout 5 ./deadlock || echo "Timeout (provável deadlock)"
run_fix: deadlock_fix
	./deadlock_fix
.PHONY: all clean run_bug run_fix
