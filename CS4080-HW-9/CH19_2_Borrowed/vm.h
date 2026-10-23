#ifndef clox_vm_h
#define clox_vm_h

//ch 19 challenge 2 (use size_t for source-buffer sizes)
#include <stddef.h>
#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

//ch 19 challenge 2 (retain each source buffer for the lifetime of its strings)
typedef struct SourceBuffer {
  char* chars;
  size_t length;
  struct SourceBuffer* next;
} SourceBuffer;

typedef struct {
  Chunk* chunk;
  uint8_t* ip;
  Value stack[STACK_MAX];
  Value* stackTop;
  Obj* objects;
  //ch 19 challenge 2 (REPL input must survive reuse of the input line buffer)
  SourceBuffer* sources;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif
