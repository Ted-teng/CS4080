//ch 20 challenge 1 (test mixed keys, deletion, growth, and interning)
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"
#include <math.h>
#include "table.h"

static void expect(Table* table, Value key, double wanted) {
  Value value;
  assert(tableGet(table, key, &value));
  assert(IS_NUMBER(value) && AS_NUMBER(value) == wanted);
}
int main(void) {
  initVM();
  Table table, copy;
  initTable(&table);
  initTable(&copy);
  Value keys[] = {NIL_VAL, BOOL_VAL(false), BOOL_VAL(true), NUMBER_VAL(0),
                  NUMBER_VAL(1), NUMBER_VAL(-1.5), NUMBER_VAL(INFINITY),
                  NUMBER_VAL(-INFINITY), NUMBER_VAL(NAN),
                  OBJ_VAL(copyString("1", 1)), OBJ_VAL(copyString("nil", 3))};
  for (int i = 0; i < 11; i++) assert(tableSet(&table, keys[i], NUMBER_VAL(i)));
  for (int i = 0; i < 11; i++) expect(&table, keys[i], i);
  assert(!tableSet(&table, NUMBER_VAL(-0.0), NUMBER_VAL(50)));
  expect(&table, NUMBER_VAL(0.0), 50);
  assert(!tableSet(&table, NUMBER_VAL(-NAN), NUMBER_VAL(60)));
  expect(&table, NUMBER_VAL(NAN), 60);
  assert(!valuesEqual(NUMBER_VAL(NAN), NUMBER_VAL(NAN)));
  assert(tableDelete(&table, NIL_VAL));
  assert(!tableDelete(&table, NIL_VAL));
  assert(tableSet(&table, NIL_VAL, NUMBER_VAL(70)));
  for (int i = 100; i < 10100; i++) tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i*2));
  for (int i = 100; i < 10100; i += 2) assert(tableDelete(&table, NUMBER_VAL(i)));
  for (int i = 101; i < 10100; i += 2) expect(&table, NUMBER_VAL(i), i*2);
  for (int i = 100; i < 10100; i += 2) assert(tableSet(&table, NUMBER_VAL(i), NIL_VAL));
  tableAddAll(&table, &copy);
  expect(&copy, NIL_VAL, 70);
  Value value;
  assert(tableGet(&copy, NUMBER_VAL(100), &value) && IS_NIL(value));
  assert(!tableGet(&copy, NUMBER_VAL(50000), &value));
  ObjString* a = copyString("same", 4);
  assert(copyString("same", 4) == a);
  assert(tableFindString(&vm.strings, "same", 4, a->hash) == a);
  assert(interpret("\"a\" + \"b\" == \"ab\"") == INTERPRET_OK);
  freeTable(&copy);
  freeTable(&table);
  assert(interpret("\"a\" + 1") == INTERPRET_RUNTIME_ERROR);
  freeVM();
  puts("Generic table tests passed.");
  return 0;
}
