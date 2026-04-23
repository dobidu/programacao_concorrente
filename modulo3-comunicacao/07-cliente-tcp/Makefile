CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = client_tcp
all: $(TARGET)
$(TARGET): client_tcp.c
	$(CC) $(CFLAGS) -o $(TARGET) client_tcp.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
