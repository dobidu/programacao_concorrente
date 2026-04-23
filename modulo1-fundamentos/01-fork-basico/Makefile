CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = fork_basico

all: $(TARGET)

$(TARGET): fork_basico.c
	$(CC) $(CFLAGS) -o $(TARGET) fork_basico.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
