#include <stdlib.h>
#include <stdio.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  chunk->lineCount = 0;
  chunk->lineCapacity = 0;
  initValueArray(&chunk->constants);
}
void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(LineStart, chunk->lines, chunk->lineCapacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code,
        oldCapacity, chunk->capacity);

  }

  chunk->code[chunk->count] = byte;
  if (chunk->lineCount == 0 ||
      chunk->lines[chunk->lineCount - 1].line != line) {
    if (chunk->lineCapacity < chunk->lineCount + 1) {
      int oldCapacity = chunk->lineCapacity;
      chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
      chunk->lines = GROW_ARRAY(LineStart, chunk->lines,
          oldCapacity, chunk->lineCapacity);
    }
    chunk->lines[chunk->lineCount++] = (LineStart){chunk->count, line};
  }
  chunk->count++;
}
int addConstant(Chunk* chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}

int getLine(Chunk* chunk, int offset) {
  if (offset < 0 || offset >= chunk->count) return -1;
  for (int i = chunk->lineCount - 1; i >= 0; i--) {
    if (offset >= chunk->lines[i].offset) return chunk->lines[i].line;
  }
  return -1;
}

// Chapter 14, Challenge 2: Use three operand bytes only when needed.
void writeConstant(Chunk* chunk, Value value, int line) {
  if (chunk->constants.count > 0xffffff) {
    fprintf(stderr, "Too many constants in one chunk.\n");
    exit(1);
  }
  int index = addConstant(chunk, value);
  if (index <= UINT8_MAX) {
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, (uint8_t)index, line);
  } else {
    writeChunk(chunk, OP_CONSTANT_LONG, line);
    writeChunk(chunk, (uint8_t)index, line);
    writeChunk(chunk, (uint8_t)(index >> 8), line);
    writeChunk(chunk, (uint8_t)(index >> 16), line);
  }
}
