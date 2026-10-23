CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter
LDLIBS = -lm
CORE = chunk.c compiler.c debug.c memory.c object.c scanner.c value.c vm.c

.PHONY: all test clean
all: clox
clox: main.c $(CORE) $(wildcard *.h)
	$(CC) $(CFLAGS) main.c $(CORE) -o $@ $(LDLIBS)
tests: tests.c $(CORE) $(wildcard *.h)
	$(CC) $(CFLAGS) tests.c $(CORE) -o $@ $(LDLIBS)
test: tests
	./tests
clean:
	rm -f clox clox.exe tests tests.exe bench_lookup bench_lookup.exe bench_strings bench_strings.exe bench_churn bench_churn.exe
