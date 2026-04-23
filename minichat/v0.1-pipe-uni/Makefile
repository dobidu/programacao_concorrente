CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = minichat
all: $(TARGET)
$(TARGET): minichat.c
	$(CC) $(CFLAGS) -o $(TARGET) minichat.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
