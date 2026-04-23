CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = trylock
all: $(TARGET)
$(TARGET): trylock.c
	$(CC) $(CFLAGS) -o $(TARGET) trylock.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
