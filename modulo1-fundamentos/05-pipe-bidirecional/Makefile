CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = pipe_bidi

all: $(TARGET)

$(TARGET): pipe_bidi.c
	$(CC) $(CFLAGS) -o $(TARGET) pipe_bidi.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
