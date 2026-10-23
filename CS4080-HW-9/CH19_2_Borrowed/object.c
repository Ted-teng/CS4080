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

//ch 19 challenge 2 (record whether this object must free its characters)
static ObjString* allocateString(const char* chars, int length, bool ownsChars) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->ownsChars = ownsChars;
  return string;
}
ObjString* takeString(char* chars, int length) {
  return allocateString(chars, length, true);
}
ObjString* copyString(const char* chars, int length) {
  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, true);
}
//ch 19 challenge 2 (create an object without copying or owning the characters)
ObjString* borrowString(const char* chars, int length) {
  return allocateString(chars, length, false);
}
void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      //ch 19 challenge 2 (borrowed source spans may not end in a null byte)
      fwrite(AS_STRING(value)->chars, 1, AS_STRING(value)->length, stdout);
      break;
  }
}
