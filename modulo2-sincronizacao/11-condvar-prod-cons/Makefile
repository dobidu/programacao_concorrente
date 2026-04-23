CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = prod_cons
all: $(TARGET)
$(TARGET): prod_cons.c
	$(CC) $(CFLAGS) -o $(TARGET) prod_cons.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
