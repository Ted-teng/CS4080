//ch 21 challenge 1 (check deduplication, byte operands, REPL reset, and limits)
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

int main(void) {
  initVM();
  Chunk chunk;
  initChunk(&chunk);
  char source[40000] = "var item = nil;";
  for (int i = 0; i < 1000; i++) strcat(source, "item = item;");
  assert(compile(source, &chunk));
  assert(chunk.constants.count == 1);
  for (int offset = 0; offset < chunk.count;) {
    uint8_t op = chunk.code[offset++];
    if (op == OP_DEFINE_GLOBAL || op == OP_GET_GLOBAL || op == OP_SET_GLOBAL) {
      assert(chunk.code[offset++] == 0);
    }
  }
  freeChunk(&chunk);
  initChunk(&chunk);
  assert(compile("var other = 123; print other;", &chunk));
  assert(chunk.constants.count == 2);
  assert(IS_STRING(chunk.constants.values[0]));
  assert(strcmp(AS_CSTRING(chunk.constants.values[0]), "other") == 0);
  freeChunk(&chunk);
  initChunk(&chunk);
  source[0] = '\0';
  for (int i = 0; i < 256; i++) {
    char declaration[40];
    sprintf(declaration, "var name%d = nil;", i);
    strcat(source, declaration);
  }
  assert(compile(source, &chunk) && chunk.constants.count == 256);
  freeChunk(&chunk);
  initChunk(&chunk);
  strcat(source, "var overflow = nil;");
  assert(!compile(source, &chunk));
  freeChunk(&chunk);
  assert(interpret("var a = 10; a = a + 1; print a;") == INTERPRET_OK);
  assert(interpret("print a;") == INTERPRET_OK);
  assert(interpret("notDefined = 1;") == INTERPRET_RUNTIME_ERROR);
  assert(interpret("print notDefined;") == INTERPRET_RUNTIME_ERROR);
  assert(interpret("var a = 20; print a;") == INTERPRET_OK);
  freeVM();
  puts("Global name tests passed.");
  return 0;
}
