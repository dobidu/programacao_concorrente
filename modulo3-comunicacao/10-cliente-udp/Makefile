CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = client_udp
all: $(TARGET)
$(TARGET): client_udp.c
	$(CC) $(CFLAGS) -o $(TARGET) client_udp.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
