CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter
LDLIBS = -lm
CORE = chunk.c compiler.c debug.c memory.c object.c scanner.c table.c value.c vm.c

.PHONY: all clean
all: clox
clox: main.c $(CORE) $(wildcard *.h)
	$(CC) $(CFLAGS) main.c $(CORE) -o $@ $(LDLIBS)
clox_baseline: main.c $(CORE) $(wildcard *.h)
	$(CC) $(CFLAGS) -DCLOX_BASELINE_IP main.c $(CORE) -o $@ $(LDLIBS)
clean:
	rm -f clox clox.exe clox_baseline clox_baseline.exe *.o
