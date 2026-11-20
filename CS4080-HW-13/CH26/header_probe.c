#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "object.h"

//ch 26 challenge 1 (measure the original header separately from RC metadata)
typedef struct OriginalObj {
  ObjType type;
  bool isMarked;
  struct OriginalObj* next;
} OriginalObj;

//ch 26 challenge 1 (a layout demonstration, not the VM's active header)
typedef struct {
  uint64_t bits;
} PackedHeader;

#define POINTER_MASK UINT64_C(0x0000ffffffffffff)
#define MARK_MASK    UINT64_C(0x0001000000000000)

#if UINTPTR_MAX == UINT64_MAX
//ch 26 challenge 1 (reject pointers that cannot fit instead of truncating them)
static bool initPacked(PackedHeader* header, ObjType type, bool mark,
                       Obj* next) {
  uint64_t address = (uint64_t)(uintptr_t)next;
  if ((address & ~POINTER_MASK) != 0) return false;
  header->bits = ((uint64_t)type << 56) |
                 ((uint64_t)mark << 48) | address;
  return true;
}

static ObjType packedType(const PackedHeader* header) {
  return (ObjType)(header->bits >> 56);
}

static bool packedMark(const PackedHeader* header) {
  return (header->bits & MARK_MASK) != 0;
}

static Obj* packedNext(const PackedHeader* header) {
  return (Obj*)(uintptr_t)(header->bits & POINTER_MASK);
}

static void setPackedMark(PackedHeader* header, bool mark) {
  header->bits = (header->bits & ~MARK_MASK) | ((uint64_t)mark << 48);
}
#endif

int main(void) {
  printf("sizeof(ObjType): %zu\n", sizeof(ObjType));
  printf("sizeof(bool): %zu\n", sizeof(bool));
  printf("sizeof(pointer): %zu\n", sizeof(Obj*));
  printf("Original header: %zu\n", sizeof(OriginalObj));
  printf("Offsets (type, mark, next): %zu, %zu, %zu\n",
         offsetof(OriginalObj, type), offsetof(OriginalObj, isMarked),
         offsetof(OriginalObj, next));
  printf("Original padding: %zu\n", sizeof(OriginalObj) - sizeof(ObjType) -
         sizeof(bool) - sizeof(Obj*));
  printf("Active Obj header: %zu\n", sizeof(Obj));
  printf("Packed demonstration: %zu\n", sizeof(PackedHeader));
#if UINTPTR_MAX == UINT64_MAX
  Obj next = {0};
  PackedHeader header;
  if (initPacked(&header, OBJ_STRING, false, &next)) {
    assert(packedType(&header) == OBJ_STRING);
    assert(packedNext(&header) == &next);
    assert(!packedMark(&header));
    setPackedMark(&header, true);
    assert(packedMark(&header));
    assert(packedNext(&header) == &next);
    assert(packedType(&header) == OBJ_STRING);
    assert(initPacked(&header, OBJ_NATIVE, false, NULL));
    assert(packedNext(&header) == NULL);
    printf("Packed field round-trip: passed\n");
  } else {
    printf("Packed field test skipped: pointer does not fit in 48 bits.\n");
  }
#else
  printf("Packed field test requires 64-bit uintptr_t.\n");
#endif
  return 0;
}
