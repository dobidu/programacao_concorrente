CC = gcc
CFLAGS = -Wall -Wextra -g
all: fifo_writer fifo_reader
fifo_writer: fifo_writer.c
	$(CC) $(CFLAGS) -o fifo_writer fifo_writer.c
fifo_reader: fifo_reader.c
	$(CC) $(CFLAGS) -o fifo_reader fifo_reader.c
clean:
	rm -f fifo_writer fifo_reader
run: all
	@echo "Terminal 1: ./fifo_writer"
	@echo "Terminal 2: ./fifo_reader"
.PHONY: all clean run
