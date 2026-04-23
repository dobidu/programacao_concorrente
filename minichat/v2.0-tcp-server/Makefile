CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = server
all: $(TARGET)
$(TARGET): server.c
	$(CC) $(CFLAGS) -o $(TARGET) server.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
