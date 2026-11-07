//ch 24 challenge 4 (use the C math library for numeric helpers)
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

VM vm; // [one]
//ch 24 challenge 3 (adapt clock to the success/result calling convention)
static bool clockNative(int argCount, Value* args) {
  args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
  return true;
}
//ch 24 challenge 3 (pass an error message back to the VM on failure)
static bool nativeError(Value* args, const char* message) {
  args[-1] = OBJ_VAL(copyString(message, (int)strlen(message)));
  return false;
}
//ch 24 challenge 4 (compute a square root after type and domain checks)
static bool sqrtNative(int argCount, Value* args) {
  if (!IS_NUMBER(args[0])) {
    return nativeError(args, "sqrt() expects a number.");
  }
  double value = AS_NUMBER(args[0]);
  if (value < 0) {
    return nativeError(args, "sqrt() expects a nonnegative number.");
  }
  args[-1] = NUMBER_VAL(sqrt(value));
  return true;
}
//ch 24 challenge 4 (return the absolute value of a number)
static bool absNative(int argCount, Value* args) {
  if (!IS_NUMBER(args[0])) {
    return nativeError(args, "abs() expects a number.");
  }
  args[-1] = NUMBER_VAL(fabs(AS_NUMBER(args[0])));
  return true;
}
//ch 24 challenge 4 (report the byte length stored in a Lox string)
static bool lenNative(int argCount, Value* args) {
  if (!IS_STRING(args[0])) {
    return nativeError(args, "len() expects a string.");
  }
  args[-1] = NUMBER_VAL(AS_STRING(args[0])->length);
  return true;
}
//ch 24 challenge 4 (describe the value types available at the end of this chapter)
static bool typeNative(int argCount, Value* args) {
  const char* name;
  if (IS_NIL(args[0])) name = "nil";
  else if (IS_BOOL(args[0])) name = "boolean";
  else if (IS_NUMBER(args[0])) name = "number";
  else if (IS_STRING(args[0])) name = "string";
  else name = "function";
  args[-1] = OBJ_VAL(copyString(name, (int)strlen(name)));
  return true;
}
static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
}
static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", // [minus]
            function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}
//ch 24 challenge 2 (register both the native function and its arity)
static void defineNative(const char* name, NativeFn function, int arity) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function, arity)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void initVM() {
  resetStack();
  vm.objects = NULL;

  initTable(&vm.globals);
  initTable(&vm.strings);

  //ch 24 challenge 2 (clock takes no arguments)
  defineNative("clock", clockNative, 0);
  //ch 24 challenge 4 (register the new one-argument native functions)
  defineNative("sqrt", sqrtNative, 1);
  defineNative("abs", absNative, 1);
  defineNative("len", lenNative, 1);
  defineNative("type", typeNative, 1);
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  freeObjects();
}
void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}
Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}
static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}
static bool call(ObjFunction* function, int argCount) {
  if (argCount != function->arity) {
    runtimeError("Expected %d arguments but got %d.",
        function->arity, argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->function = function;
  frame->ip = function->chunk.code;
  frame->slots = vm.stackTop - argCount - 1;
  return true;
}
static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_FUNCTION: // [switch]
        return call(AS_FUNCTION(callee), argCount);
      case OBJ_NATIVE: {
        //ch 24 challenge 2 (check arity before the native reads any arguments)
        ObjNative* native = (ObjNative*)AS_OBJ(callee);
        if (argCount != native->arity) {
          runtimeError("Expected %d arguments but got %d.",
              native->arity, argCount);
          return false;
        }
        //ch 24 challenge 3 (leave a successful result in the former callee slot)
        Value* args = vm.stackTop - argCount;
        if (!native->function(argCount, args)) {
          runtimeError("%s", AS_CSTRING(args[-1]));
          return false;
        }
        vm.stackTop -= argCount;
        return true;
      }
      default:
        break; // Non-callable object type.
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}
static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
static void concatenate() {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  push(OBJ_VAL(result));
}
static InterpretResult run() {
  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  //ch 24 challenge 1 (keep ip local; the alternate build supplies a benchmark baseline)
#ifdef CLOX_BASELINE_IP
#define IP (frame->ip)
#else
  register uint8_t* ip = frame->ip;
#define IP ip
#endif
#define SAVE_IP() (frame->ip = IP)
#define LOAD_IP() (IP = frame->ip)

#define READ_BYTE() (*IP++)

#define READ_SHORT() \
    (IP += 2, \
    (uint16_t)((IP[-2] << 8) | IP[-1]))

#define READ_CONSTANT() \
    (frame->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
//ch 24 challenge 1 (synchronize the local ip before reporting runtime errors)
#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        SAVE_IP(); \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(&frame->function->chunk,
        (int)(IP - frame->function->chunk.code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
      case OP_POP: pop(); break;
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        Value value;
        if (!tableGet(&vm.globals, name, &value)) {
          SAVE_IP();
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        tableSet(&vm.globals, name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (tableSet(&vm.globals, name, peek(0))) {
          tableDelete(&vm.globals, name); // [delete]
          SAVE_IP();
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          SAVE_IP();
          runtimeError(
              "Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          SAVE_IP();
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      case OP_PRINT: {
        printValue(pop());
        printf("\n");
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        IP += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) IP += offset;
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        IP -= offset;
        break;
      }
      case OP_CALL: {
        int argCount = READ_BYTE();
        //ch 24 challenge 1 (save the caller position for calls and error traces)
        SAVE_IP();
        if (!callValue(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        //ch 24 challenge 1 (load the callee or the resumed caller position)
        LOAD_IP();
        break;
      }
      case OP_RETURN: {
        Value result = pop();
        vm.frameCount--;
        if (vm.frameCount == 0) {
          pop();
          return INTERPRET_OK;
        }

        vm.stackTop = frame->slots;
        push(result);
        frame = &vm.frames[vm.frameCount - 1];
        //ch 24 challenge 1 (load the callee or the resumed caller position)
        LOAD_IP();
        break;
      }
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
#undef SAVE_IP
#undef LOAD_IP
#undef IP
}
void hack(bool b) {
  // Hack to avoid unused function error. run() is not used in the
  // scanning chapter.
  run();
  if (b) hack(false);
}
InterpretResult interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  call(function, 0);

  return run();
}
