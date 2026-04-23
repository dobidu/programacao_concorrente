CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = barreira
all: $(TARGET)
$(TARGET): barreira.c
	$(CC) $(CFLAGS) -o $(TARGET) barreira.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
