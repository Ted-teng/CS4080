//ch 19 challenge 1 (check concatenated length before allocating)
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm.objects;
  vm.objects = object;
  return object;
}

//ch 19 challenge 1 (allocate the header, characters, and terminator together)
static ObjString* allocateString(int length) {
  ObjString* string = (ObjString*)allocateObject(
      sizeof(ObjString) + (size_t)length + 1, OBJ_STRING);
  string->length = length;
  string->chars[length] = '\0';
  return string;
}
//ch 19 challenge 1 (keep takeString's ownership contract with inline storage)
ObjString* takeString(char* chars, int length) {
  ObjString* string = copyString(chars, length);
  FREE_ARRAY(char, chars, length + 1);
  return string;
}
ObjString* copyString(const char* chars, int length) {
  //ch 19 challenge 1 (copy directly into the object's flexible array)
  ObjString* string = allocateString(length);
  memcpy(string->chars, chars, length);
  return string;
}
//ch 19 challenge 1 (fill the result in its final single allocation)
ObjString* concatenateStrings(ObjString* a, ObjString* b) {
  if (a->length > INT_MAX - b->length) exit(1);
  ObjString* string = allocateString(a->length + b->length);
  memcpy(string->chars, a->chars, a->length);
  memcpy(string->chars + a->length, b->chars, b->length);
  return string;
}
void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}
