CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = atomics
all: $(TARGET)
$(TARGET): atomics.c
	$(CC) $(CFLAGS) -o $(TARGET) atomics.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
