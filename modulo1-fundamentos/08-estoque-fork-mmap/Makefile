CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = estoque

all: $(TARGET)

$(TARGET): estoque.c
	$(CC) $(CFLAGS) -o $(TARGET) estoque.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
