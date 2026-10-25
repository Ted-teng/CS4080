CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter
CORE = chunk.c compiler.c debug.c localmap.c memory.c object.c scanner.c table.c value.c vm.c
HEADERS = $(wildcard *.h)

.PHONY: clean

clox: main.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) main.c $(CORE) -o clox

clox_linear: main.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) -DCLOX_LINEAR_LOCALS main.c $(CORE) -o clox_linear

clean:
	rm -f clox clox_linear *.exe *.o
