CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = mutex_basico
all: $(TARGET)
$(TARGET): mutex_basico.c
	$(CC) $(CFLAGS) -o $(TARGET) mutex_basico.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
