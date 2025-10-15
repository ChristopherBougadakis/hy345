CC = gcc
CFLAGS = -Wall -Wextra -g

all: cell

cell: cell.c cell.h
	$(CC) $(CFLAGS) -o cell cell.c 

clean:
	rm -f cell

.PHONY: all clean