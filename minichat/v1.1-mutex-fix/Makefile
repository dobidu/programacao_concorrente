CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = minichat_fix
all: $(TARGET)
$(TARGET): minichat_fix.c
	$(CC) $(CFLAGS) -o $(TARGET) minichat_fix.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
