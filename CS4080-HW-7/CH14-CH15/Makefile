CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra
CORE = chunk.c debug.c memory.c value.c vm.c

clox: main.c $(CORE)
	$(CC) $(CFLAGS) main.c $(CORE) -o clox

test: tests.c $(CORE)
	$(CC) $(CFLAGS) -DCLOX_BENCHMARK tests.c $(CORE) -o tests
	./tests

bench: bench.c $(CORE)
	$(CC) $(CFLAGS) -DCLOX_BENCHMARK bench.c $(CORE) -o bench
	$(CC) $(CFLAGS) -DCLOX_BENCHMARK -DCLOX_BASELINE_NEGATE bench.c $(CORE) -o bench_baseline

.PHONY: test bench
