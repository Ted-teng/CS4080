CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter
SOURCES = main.c chunk.c compiler.c debug.c memory.c object.c scanner.c table.c value.c vm.c
HEADERS = $(wildcard *.h)

.PHONY: all test benchmark clean
all: clox clox_baseline
clox: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $@
clox_baseline: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DCLOX_ALWAYS_WRAP $(SOURCES) -o $@
test: all
	python3 tests.py
benchmark: all
	python3 benchmarks/run.py
clean:
	rm -f clox clox.exe clox_baseline clox_baseline.exe clox_sanitize clox_sanitize.exe
