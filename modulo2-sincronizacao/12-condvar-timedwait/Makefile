CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = timedwait
all: $(TARGET)
$(TARGET): timedwait.c
	$(CC) $(CFLAGS) -o $(TARGET) timedwait.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
