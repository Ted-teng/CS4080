#include <string.h>

#include "localmap.h"
#include "memory.h"

// Chapter 22, Challenge 1: Keys borrow text from the current source.
// A slot of -1 is empty; -2 marks a deleted entry.
static uint32_t hashName(const Token* name) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < name->length; i++) {
    hash ^= (uint8_t)name->start[i];
    hash *= 16777619u;
  }
  return hash;
}

void initLocalMap(LocalMap* map) {
  map->count = 0;
  map->used = 0;
  map->capacity = 0;
  map->entries = NULL;
}

void freeLocalMap(LocalMap* map) {
  FREE_ARRAY(LocalEntry, map->entries, map->capacity);
  initLocalMap(map);
}

static LocalEntry* findEntry(LocalEntry* entries, int capacity,
                             const Token* name, uint32_t hash) {
  uint32_t index = hash & (uint32_t)(capacity - 1);
  LocalEntry* tombstone = NULL;
  for (;;) {
    LocalEntry* entry = &entries[index];
    if (entry->slot == -1) return tombstone != NULL ? tombstone : entry;
    if (entry->slot == -2) {
      if (tombstone == NULL) tombstone = entry;
    } else if (entry->hash == hash && entry->name.length == name->length &&
               memcmp(entry->name.start, name->start, name->length) == 0) {
      return entry;
    }
    index = (index + 1) & (uint32_t)(capacity - 1);
  }
}

static void adjustCapacity(LocalMap* map, int capacity) {
  LocalEntry* entries = ALLOCATE(LocalEntry, capacity);
  for (int i = 0; i < capacity; i++) entries[i].slot = -1;
  for (int i = 0; i < map->capacity; i++) {
    LocalEntry* old = &map->entries[i];
    if (old->slot < 0) continue;
    *findEntry(entries, capacity, &old->name, old->hash) = *old;
  }
  FREE_ARRAY(LocalEntry, map->entries, map->capacity);
  map->entries = entries;
  map->capacity = capacity;
  map->used = map->count;
}

int getLocalMap(LocalMap* map, const Token* name) {
  if (map->count == 0) return -1;
  LocalEntry* entry = findEntry(map->entries, map->capacity, name, hashName(name));
  return entry->slot >= 0 ? entry->slot : -1;
}

void setLocalMap(LocalMap* map, Token name, int slot) {
  if (map->used + 1 > map->capacity * 3 / 4) {
    // Reuse the capacity when deleted entries caused the table to fill.
    int capacity = map->capacity;
    if (capacity == 0 || map->count + 1 > capacity / 2) {
      capacity = GROW_CAPACITY(capacity);
    }
    adjustCapacity(map, capacity);
  }
  uint32_t hash = hashName(&name);
  LocalEntry* entry = findEntry(map->entries, map->capacity, &name, hash);
  if (entry->slot < 0) {
    map->count++;
    if (entry->slot == -1) map->used++;
  }
  entry->name = name;
  entry->hash = hash;
  entry->slot = slot;
}

void deleteLocalMap(LocalMap* map, const Token* name) {
  if (map->count == 0) return;
  LocalEntry* entry = findEntry(map->entries, map->capacity, name, hashName(name));
  if (entry->slot < 0) return;
  entry->slot = -2;
  map->count--;
}
