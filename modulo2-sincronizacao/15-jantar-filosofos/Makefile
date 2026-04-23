CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = filosofos
all: $(TARGET)
$(TARGET): filosofos.c
	$(CC) $(CFLAGS) -o $(TARGET) filosofos.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
