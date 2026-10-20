CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter
PYTHON = python3
CORE = chunk.c compiler.c debug.c memory.c scanner.c value.c
HEADERS = $(wildcard *.h)

clox_parse: main.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) main.c $(CORE) -o clox_parse

parser_structure: tests/parser_structure.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) -I. tests/parser_structure.c $(filter-out compiler.c,$(CORE)) -o parser_structure

test: clox_parse parser_structure
	./parser_structure
	$(PYTHON) tests/test_parser.py ./clox_parse

clean:
	rm -f clox_parse parser_structure *.exe *.o

.PHONY: test clean
