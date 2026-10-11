#include <stdio.h>
#include <stdlib.h>

#include "memory.h"

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm; // [one]
static void resetStack() {
  vm.stackCount = 0;
}

void initVM() {
  vm.stack = NULL;
  vm.stackCapacity = 0;
  resetStack();
}

void freeVM() {
  FREE_ARRAY(Value, vm.stack, vm.stackCapacity);
  vm.stack = NULL;
  vm.stackCapacity = 0;
  resetStack();
}
void push(Value value) {
  if (vm.stackCount == vm.stackCapacity) {
    size_t oldCapacity = vm.stackCapacity;
    if (oldCapacity > SIZE_MAX / sizeof(Value) / 2) {
      fprintf(stderr, "Stack is too large.\n");
      exit(1);
    }
    vm.stackCapacity = GROW_CAPACITY(oldCapacity);
    vm.stack = GROW_ARRAY(Value, vm.stack, oldCapacity, vm.stackCapacity);
  }
  vm.stack[vm.stackCount++] = value;
}
Value pop() {
  return vm.stack[--vm.stackCount];
}
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
    do { \
      double b = pop(); \
      double a = pop(); \
      push(a op b); \
    } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stack + vm.stackCount; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk,
                           (int)(vm.ip - vm.chunk->code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_CONSTANT_LONG: {
        uint32_t index = READ_BYTE();
        index |= (uint32_t)READ_BYTE() << 8;
        index |= (uint32_t)READ_BYTE() << 16;
        push(vm.chunk->constants.values[index]);
        break;
      }
      case OP_ADD:      BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE:   BINARY_OP(/); break;
      case OP_NEGATE:
        // Chapter 15, Challenge 4: Keep the stack height unchanged.
#ifdef CLOX_BASELINE_NEGATE
        push(-pop());
#else
        vm.stack[vm.stackCount - 1] = -vm.stack[vm.stackCount - 1];
#endif
        break;
      case OP_RETURN: {
#ifdef CLOX_BENCHMARK
        vm.result = pop();
#else
        printValue(pop());
        printf("\n");
#endif
        return INTERPRET_OK;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}
void hack(bool b) {
  // Hack to avoid unused function error. run() is not used in the
  // scanning chapter.
  run();
  if (b) hack(false);
}
InterpretResult interpret(Chunk* chunk) {
  resetStack();
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}
