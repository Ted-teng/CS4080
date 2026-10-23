//ch 19 challenge 1 (verify inline storage, concatenation, and ownership transfer)
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

int main(void) {
  initVM();
  ObjString* a = copyString("hello", 5);
  assert(a->chars == (char*)a + offsetof(ObjString, chars));
  assert(a->length == 5 && strcmp(a->chars, "hello") == 0);
  ObjString* empty = copyString("", 0);
  ObjString* result = concatenateStrings(a, empty);
  assert(result != a && strcmp(result->chars, "hello") == 0);
  ObjString* b = copyString(" world", 6);
  result = concatenateStrings(a, b);
  assert(result->length == 11 && strcmp(result->chars, "hello world") == 0);
  char* owned = ALLOCATE(char, 4);
  memcpy(owned, "abc", 4);
  assert(strcmp(takeString(owned, 3)->chars, "abc") == 0);
  assert(interpret("\"a\" + \"b\"") == INTERPRET_OK);
  assert(interpret("\"a\" + 1") == INTERPRET_RUNTIME_ERROR);
  freeVM();
  puts("Flexible string tests passed.");
  return 0;
}
