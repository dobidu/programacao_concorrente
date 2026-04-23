CC = gcc
CFLAGS = -Wall -Wextra -lrt -g
TARGET = shm_demo
all: $(TARGET)
$(TARGET): shm_demo.c
	$(CC) $(CFLAGS) -o $(TARGET) shm_demo.c
clean:
	rm -f $(TARGET)
.PHONY: all clean
