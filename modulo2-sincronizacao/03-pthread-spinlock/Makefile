CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = pthread_spin
all: $(TARGET)
$(TARGET): pthread_spin.c
	$(CC) $(CFLAGS) -o $(TARGET) pthread_spin.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
