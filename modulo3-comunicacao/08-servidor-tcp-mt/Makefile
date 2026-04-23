CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = server_mt
all: $(TARGET)
$(TARGET): server_mt.c
	$(CC) $(CFLAGS) -o $(TARGET) server_mt.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
