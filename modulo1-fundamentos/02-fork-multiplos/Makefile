CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = fork_multiplos

all: $(TARGET)

$(TARGET): fork_multiplos.c
	$(CC) $(CFLAGS) -o $(TARGET) fork_multiplos.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
