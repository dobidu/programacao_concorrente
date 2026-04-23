CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = pipe_basico

all: $(TARGET)

$(TARGET): pipe_basico.c
	$(CC) $(CFLAGS) -o $(TARGET) pipe_basico.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
