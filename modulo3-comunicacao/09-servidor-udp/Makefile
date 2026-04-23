CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = server_udp
all: $(TARGET)
$(TARGET): server_udp.c
	$(CC) $(CFLAGS) -o $(TARGET) server_udp.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
