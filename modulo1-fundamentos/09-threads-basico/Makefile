CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = threads_basico

all: $(TARGET)

$(TARGET): threads_basico.c
	$(CC) $(CFLAGS) -o $(TARGET) threads_basico.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
