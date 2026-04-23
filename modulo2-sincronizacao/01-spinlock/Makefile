CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = spinlock
all: $(TARGET)
$(TARGET): spinlock.c
	$(CC) $(CFLAGS) -o $(TARGET) spinlock.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
