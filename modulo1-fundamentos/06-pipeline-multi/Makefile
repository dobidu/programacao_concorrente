CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = pipeline

all: $(TARGET)

$(TARGET): pipeline.c
	$(CC) $(CFLAGS) -o $(TARGET) pipeline.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
