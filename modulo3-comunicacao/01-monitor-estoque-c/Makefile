CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = monitor
all: $(TARGET)
$(TARGET): monitor.c
	$(CC) $(CFLAGS) -o $(TARGET) monitor.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
