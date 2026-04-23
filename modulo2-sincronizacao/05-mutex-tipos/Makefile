CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = mutex_recursive
all: $(TARGET)
$(TARGET): mutex_recursive.c
	$(CC) $(CFLAGS) -o $(TARGET) mutex_recursive.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
