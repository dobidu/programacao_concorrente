CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = ticket_lock
all: $(TARGET)
$(TARGET): ticket_lock.c
	$(CC) $(CFLAGS) -o $(TARGET) ticket_lock.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
