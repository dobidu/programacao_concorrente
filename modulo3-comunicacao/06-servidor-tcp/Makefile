CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = server_tcp
all: $(TARGET)
$(TARGET): server_tcp.c
	$(CC) $(CFLAGS) -o $(TARGET) server_tcp.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
