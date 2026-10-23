#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

//ch 20 challenge 1 (keep empty and deleted states separate from every valid key)
typedef enum { ENTRY_EMPTY, ENTRY_FULL, ENTRY_DELETED } EntryState;

typedef struct {
  Value key;
  EntryState state;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
//ch 20 challenge 1 (accept tagged primitive keys in the public table API)
bool tableGet(Table* table, Value key, Value* value);
bool tableSet(Table* table, Value key, Value value);
bool tableDelete(Table* table, Value key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash);

#endif
