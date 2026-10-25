#ifndef clox_localmap_h
#define clox_localmap_h

#include "common.h"
#include "scanner.h"

typedef struct {
  Token name;
  uint32_t hash;
  int slot;
} LocalEntry;

typedef struct {
  int count;
  int used;
  int capacity;
  LocalEntry* entries;
} LocalMap;

void initLocalMap(LocalMap* map);
void freeLocalMap(LocalMap* map);
int getLocalMap(LocalMap* map, const Token* name);
void setLocalMap(LocalMap* map, Token name, int slot);
void deleteLocalMap(LocalMap* map, const Token* name);

#endif
