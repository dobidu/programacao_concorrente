CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = threads_retorno
all: $(TARGET)
$(TARGET): threads_retorno.c
	$(CC) $(CFLAGS) -o $(TARGET) threads_retorno.c
clean:
	rm -f $(TARGET)
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean run
