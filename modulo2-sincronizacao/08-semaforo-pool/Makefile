CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = sem_pool
all: $(TARGET)
$(TARGET): sem_pool.c
	$(CC) $(CFLAGS) -o $(TARGET) sem_pool.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
