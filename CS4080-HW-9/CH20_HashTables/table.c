//ch 20 challenge 1 (normalize NaNs and signed zero when hashing numbers)
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

//ch 20 challenge 1 (equal numeric keys must have the same hash)
static uint32_t hashValue(Value key) {
  switch (key.type) {
    case VAL_BOOL: return AS_BOOL(key) ? 3u : 5u;
    case VAL_NIL: return 7u;
    case VAL_NUMBER: {
      double number = AS_NUMBER(key);
      if (number == 0) return 0;
      if (isnan(number)) return 0x7fc00000u;
      //ch 20 challenge 1 (copy bits without violating C aliasing rules)
      typedef char DoubleMustBe64Bits[sizeof(double) == sizeof(uint64_t) ? 1 : -1];
      (void)sizeof(DoubleMustBe64Bits);
      uint64_t bits;
      memcpy(&bits, &number, sizeof(bits));
      bits ^= bits >> 33;
      bits *= UINT64_C(0xff51afd7ed558ccd);
      bits ^= bits >> 33;
      bits *= UINT64_C(0xc4ceb9fe1a85ec53);
      bits ^= bits >> 33;
      return (uint32_t)(bits ^ (bits >> 32));
    }
    case VAL_OBJ: return AS_STRING(key)->hash;
  }
  return 0;
}
//ch 20 challenge 1 (treat all NaNs as one retrievable key, without changing ==)
static bool keysEqual(Value a, Value b) {
  if (IS_NUMBER(a) && IS_NUMBER(b) &&
      isnan(AS_NUMBER(a)) && isnan(AS_NUMBER(b))) return true;
  return valuesEqual(a, b);
}

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}
void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}
// NOTE: The "Optimization" chapter has a manual copy of this function.
// If you change it here, make sure to update that copy.
static Entry* findEntry(Entry* entries, int capacity,
                        Value key) {
  //ch 20 challenge 1 (hash the tagged key instead of assuming a string)
  uint32_t index = hashValue(key) % capacity;
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = &entries[index];
    if (entry->state != ENTRY_FULL) {
      if (entry->state == ENTRY_EMPTY) {
        // Empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (keysEqual(entry->key, key)) {
      // We found the key.
      return entry;
    }

    index = (index + 1) % capacity;
  }
}
bool tableGet(Table* table, Value key, Value* value) {
  if (table->count == 0) return false;

  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->state != ENTRY_FULL) return false;

  *value = entry->value;
  return true;
}
static void adjustCapacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    //ch 20 challenge 1 (nil is a valid key, not an empty-slot marker)
    entries[i].state = ENTRY_EMPTY;
    entries[i].key = NIL_VAL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->state != ENTRY_FULL) continue;

    Entry* dest = findEntry(entries, capacity, entry->key);
    //ch 20 challenge 1 (rehash only live entries)
    dest->state = ENTRY_FULL;
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}
bool tableSet(Table* table, Value key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  Entry* entry = findEntry(table->entries, table->capacity, key);
  //ch 20 challenge 1 (count tracks live entries plus tombstones, as in the book)
  bool isNewKey = entry->state != ENTRY_FULL;
  if (isNewKey && entry->state == ENTRY_EMPTY) table->count++;
  entry->state = ENTRY_FULL;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}
bool tableDelete(Table* table, Value key) {
  if (table->count == 0) return false;

  // Find the entry.
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->state != ENTRY_FULL) return false;

  // Place a tombstone in the entry.
  //ch 20 challenge 1 (a deleted entry must not stop a probe sequence)
  entry->state = ENTRY_DELETED;
  entry->key = NIL_VAL;
  entry->value = NIL_VAL;
  return true;
}
void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->state == ENTRY_FULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash) {
  if (table->count == 0) return NULL;

  uint32_t index = hash % table->capacity;
  for (;;) {
    Entry* entry = &table->entries[index];
    if (entry->state != ENTRY_FULL) {
      // Stop if we find an empty non-tombstone entry.
      if (entry->state == ENTRY_EMPTY) return NULL;
    //ch 20 challenge 1 (interning searches only string keys in a generic table)
    } else if (IS_STRING(entry->key) &&
        AS_STRING(entry->key)->length == length &&
        AS_STRING(entry->key)->hash == hash &&
        memcmp(AS_STRING(entry->key)->chars, chars, length) == 0) {
      // We found it.
      return AS_STRING(entry->key);
    }

    index = (index + 1) % table->capacity;
  }
}
