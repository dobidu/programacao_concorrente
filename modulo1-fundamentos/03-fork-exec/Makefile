CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = fork_exec

all: $(TARGET)

$(TARGET): fork_exec.c
	$(CC) $(CFLAGS) -o $(TARGET) fork_exec.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
