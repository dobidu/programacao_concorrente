CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = minichat_bug
all: $(TARGET)
$(TARGET): minichat_bug.c
	$(CC) $(CFLAGS) -o $(TARGET) minichat_bug.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
