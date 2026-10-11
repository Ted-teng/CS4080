#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"


typedef struct {
  Chunk* chunk;
  uint8_t* ip;
  // Chapter 15, Challenge 3: Indices remain valid when the stack moves.
  Value* stack;
  size_t stackCount;
  size_t stackCapacity;
#ifdef CLOX_BENCHMARK
  Value result;
#endif
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);
void push(Value value);
Value pop();

#endif
