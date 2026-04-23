CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = rwlock
all: $(TARGET)
$(TARGET): rwlock.c
	$(CC) $(CFLAGS) -o $(TARGET) rwlock.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
