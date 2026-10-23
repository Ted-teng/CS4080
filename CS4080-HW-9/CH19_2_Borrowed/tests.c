//ch 19 challenge 2 (test non-terminated spans, equality, and source lifetimes)
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

int main(void) {
  initVM();
  static const char text[] = "teapot";
  ObjString* borrowed = borrowString(text, 3);
  assert(!borrowed->ownsChars && borrowed->chars == text);
  assert(valuesEqual(OBJ_VAL(borrowed), OBJ_VAL(copyString("tea", 3))));
  assert(!valuesEqual(OBJ_VAL(borrowed), OBJ_VAL(copyString("teapot", 6))));
  assert(copyString("owned", 5)->ownsChars);
  printObject(OBJ_VAL(borrowed));
  puts("");
  char input[] = "\"retained\"";
  assert(interpret(input) == INTERPRET_OK);
  ObjString* literal = (ObjString*)vm.objects;
  assert(!literal->ownsChars && literal->length == 8);
  memset(input, 'x', sizeof(input) - 1);
  assert(interpret("\"another\"") == INTERPRET_OK);
  assert(memcmp(literal->chars, "retained", 8) == 0);
  assert(interpret("\"tea\" + \"pot\"") == INTERPRET_OK);
  assert(interpret("\"tea\" == \"tea\"") == INTERPRET_OK);
  assert(interpret("\"unfinished") == INTERPRET_COMPILE_ERROR);
  assert(interpret("\"a\" + 1") == INTERPRET_RUNTIME_ERROR);
  freeVM();
  puts("Borrowed string tests passed.");
  return 0;
}
