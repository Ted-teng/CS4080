CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter
CORE = chunk.c compiler.c debug.c memory.c object.c scanner.c table.c value.c vm.c
HEADERS = $(wildcard *.h)

.PHONY: all test clean
all: clox clox_baseline clox_rc header_probe
clox: main.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) main.c $(CORE) -o $@
clox_baseline: main.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) -DCLOX_BASELINE_MARK main.c $(CORE) -o $@
clox_rc: main.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) -DCLOX_REFERENCE_COUNTS main.c $(CORE) -o $@
header_probe: header_probe.c $(HEADERS)
	$(CC) $(CFLAGS) header_probe.c -o $@
test_epoch: tests.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) -DCLOX_TEST_GC -DCLOX_STRESS_GC tests.c $(CORE) -o $@
test_baseline: tests.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) -DCLOX_TEST_GC -DCLOX_STRESS_GC -DCLOX_BASELINE_MARK tests.c $(CORE) -o $@
test_rc: tests.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) -DCLOX_TEST_GC -DCLOX_STRESS_GC -DCLOX_REFERENCE_COUNTS tests.c $(CORE) -o $@
test: all test_epoch test_baseline test_rc
	./header_probe
	./test_epoch
	./test_baseline
	./test_rc
	python3 test_examples.py
clean:
	rm -f clox clox_baseline clox_rc header_probe test_epoch test_baseline test_rc test_sanitize test_sanitize_rc *.exe
