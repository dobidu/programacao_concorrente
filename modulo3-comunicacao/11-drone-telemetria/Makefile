CC = gcc
CFLAGS = -Wall -Wextra -g
all: drone central
drone: drone.c
	$(CC) $(CFLAGS) -o drone drone.c
central: central.c
	$(CC) $(CFLAGS) -o central central.c
clean:
	rm -f drone central
.PHONY: all clean
