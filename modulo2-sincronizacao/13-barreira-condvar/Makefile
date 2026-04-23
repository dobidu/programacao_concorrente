CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = barreira_cv
all: $(TARGET)
$(TARGET): barreira_cv.c
	$(CC) $(CFLAGS) -o $(TARGET) barreira_cv.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
