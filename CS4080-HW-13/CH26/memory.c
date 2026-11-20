#include <stdlib.h>
//ch 26 challenge 3 (check reference-count invariants)
#include <assert.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

#ifdef CLOX_REFERENCE_COUNTS
//ch 26 challenge 3 (reuse the book's root and edge visitors in three modes)
typedef enum {
  TRACE_REFERENCES,
  COUNT_REFERENCES,
  RELEASE_REFERENCES
} ReferenceMode;
static ReferenceMode referenceMode = TRACE_REFERENCES;
#endif

#ifdef CLOX_TEST_GC
//ch 26 challenge 3 (test counters; excluded from normal builds)
size_t referenceFreed = 0;
size_t sweepFreed = 0;
#endif

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  vm.bytesAllocated += newSize - oldSize;
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif

    if (vm.bytesAllocated > vm.nextGC) {
      collectGarbage();
    }
  }

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
  if (result == NULL) exit(1);
  return result;
}
//ch 26 challenge 3 (share the worklist without triggering recursive GC)
static void pushGray(Obj* object) {
  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
    vm.grayStack = (Obj**)realloc(vm.grayStack,
                                  sizeof(Obj*) * vm.grayCapacity);

    if (vm.grayStack == NULL) exit(1);
  }

  vm.grayStack[vm.grayCount++] = object;
}

void markObject(Obj* object) {
  if (object == NULL) return;
#ifdef CLOX_REFERENCE_COUNTS
  //ch 26 challenge 3 (count edges, or remove edges from zero-count objects)
  if (referenceMode == COUNT_REFERENCES) {
    object->references++;
    return;
  }
  if (referenceMode == RELEASE_REFERENCES) {
    assert(object->references > 0);
    if (--object->references == 0) pushGray(object);
    return;
  }
#endif
  //ch 26 challenge 2 (a matching color means already marked)
  if (object->mark == vm.markValue) return;

#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

  //ch 26 challenge 2 (mark with this collection's color)
  object->mark = vm.markValue;

  pushGray(object);
}
void markValue(Value value) {
  if (IS_OBJ(value)) markObject(AS_OBJ(value));
}
static void markArray(ValueArray* array) {
  for (int i = 0; i < array->count; i++) {
    markValue(array->values[i]);
  }
}
static void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

  switch (object->type) {
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      markObject((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      markObject((Obj*)function->name);
      markArray(&function->chunk.constants);
      break;
    }
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)object)->closed);
      break;
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}
static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif

  switch (object->type) {
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues,
                 closure->upvalueCount);
      FREE(ObjClosure, object);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      freeChunk(&function->chunk);
      FREE(ObjFunction, object);
      break;
    }
    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
  }
}
static void markRoots() {
  for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
    markValue(*slot);
  }

  for (int i = 0; i < vm.frameCount; i++) {
    markObject((Obj*)vm.frames[i].closure);
  }

  for (ObjUpvalue* upvalue = vm.openUpvalues;
       upvalue != NULL;
       upvalue = upvalue->next) {
    markObject((Obj*)upvalue);
  }

  markTable(&vm.globals);
  markCompilerRoots();
}
static void traceReferences() {
  while (vm.grayCount > 0) {
    Obj* object = vm.grayStack[--vm.grayCount];
    blackenObject(object);
  }
}
static void sweep() {
  Obj* previous = NULL;
  Obj* object = vm.objects;
  while (object != NULL) {
    //ch 26 challenge 2 (survivors keep their mark; no clearing store)
    if (object->mark == vm.markValue) {
#ifdef CLOX_BASELINE_MARK
      object->mark = false;
#endif
      previous = object;
      object = object->next;
    } else {
      Obj* unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm.objects = object;
      }

#ifdef CLOX_TEST_GC
      //ch 26 challenge 3 (record what the tracing collector reclaimed)
      sweepFreed++;
#endif
      freeObject(unreached);
    }
  }
}
#ifdef CLOX_REFERENCE_COUNTS
//ch 26 challenge 3 (reclaim acyclic garbage with batch reference counting)
static void collectZeroReferences() {
  assert(vm.grayCount == 0);
  for (Obj* object = vm.objects; object != NULL; object = object->next) {
    object->references = 0;
  }

  //ch 26 challenge 3 (count roots and all strong object-to-object edges)
  referenceMode = COUNT_REFERENCES;
  markRoots();
  for (Obj* object = vm.objects; object != NULL; object = object->next) {
    blackenObject(object);
  }

  //ch 26 challenge 3 (zero counts cascade through unreachable chains)
  for (Obj* object = vm.objects; object != NULL; object = object->next) {
    if (object->references == 0) pushGray(object);
  }
  referenceMode = RELEASE_REFERENCES;
  traceReferences();
  referenceMode = TRACE_REFERENCES;

  //ch 26 challenge 3 (unlink garbage after all outgoing edges are visited)
  Obj** link = &vm.objects;
  while (*link != NULL) {
    Obj* object = *link;
    if (object->references == 0) {
      *link = object->next;
      if (object->type == OBJ_STRING) {
        tableDelete(&vm.strings, (ObjString*)object);
      }
#ifdef CLOX_TEST_GC
      referenceFreed++;
#endif
      freeObject(object);
    } else {
      link = &object->next;
    }
  }
}
#endif

void collectGarbage() {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm.bytesAllocated;
#endif

#ifdef CLOX_TEST_GC
  //ch 26 challenge 3 (keep per-collection counts for the tests)
  referenceFreed = 0;
  sweepFreed = 0;
#endif
#ifdef CLOX_REFERENCE_COUNTS
  //ch 26 challenge 3 (reference counts first; tracing still handles cycles)
  collectZeroReferences();
#endif
  markRoots();
  traceReferences();
  tableRemoveWhite(&vm.strings);
  sweep();

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
#ifndef CLOX_BASELINE_MARK
  //ch 26 challenge 2 (one flip makes every survivor logically unmarked)
  vm.markValue = !vm.markValue;
#endif

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated,
         vm.nextGC);
#endif
}
void freeObjects() {
  Obj* object = vm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }

  free(vm.grayStack);
}
