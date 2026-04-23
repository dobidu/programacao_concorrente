CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = sem_named
all: $(TARGET)
$(TARGET): sem_named.c
	$(CC) $(CFLAGS) -o $(TARGET) sem_named.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
