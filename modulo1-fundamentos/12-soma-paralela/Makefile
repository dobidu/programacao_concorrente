CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = soma_paralela

all: $(TARGET)

$(TARGET): soma_paralela.c
	$(CC) $(CFLAGS) -o $(TARGET) soma_paralela.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
