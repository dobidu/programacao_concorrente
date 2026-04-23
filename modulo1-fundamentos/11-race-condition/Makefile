CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = race_condition

all: $(TARGET)

$(TARGET): race_condition.c
	$(CC) $(CFLAGS) -o $(TARGET) race_condition.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
