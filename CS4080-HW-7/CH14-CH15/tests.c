#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "chunk.h"
#include "debug.h"
#include "vm.h"
extern VM vm;
static void expression(const int* sequence, int count, double expected) {
  Chunk c;
  initChunk(&c);
  for (int i = 0; i < count; i++) {
    if (sequence[i] >= 0) writeConstant(&c, sequence[i], 1);
    else writeChunk(&c, (uint8_t)-sequence[i], 1);
  }
  writeChunk(&c, OP_RETURN, 1);
  assert(interpret(&c) == INTERPRET_OK);
  assert(fabs(vm.result - expected) < 1e-12);
  freeChunk(&c);
}
int main(void) {
  Chunk c;
  initChunk(&c);
  assert(getLine(&c, 0) == -1);
  for (int i = 0; i < 100; i++) writeChunk(&c, OP_NEGATE, 10);
  writeChunk(&c, OP_RETURN, 20);
  assert(c.lineCount == 2);
  assert(getLine(&c, 0) == 10 && getLine(&c, 99) == 10);
  assert(getLine(&c, 100) == 20 && getLine(&c, 101) == -1);
  freeChunk(&c);
  initVM();
  for (int i = 0; i < 10000; i++) push(i);
  for (int i = 9999; i >= 0; i--) assert(pop() == i);
  for (int i = 0; i < 65536; i++) addConstant(&c, i);
  writeConstant(&c, 123.5, 30);
  assert(c.code[0] == OP_CONSTANT_LONG);
  assert(c.code[1] == 0 && c.code[2] == 0 && c.code[3] == 1);
  assert(disassembleInstruction(&c, 0) == 4);
  writeChunk(&c, OP_RETURN, 30);
  assert(interpret(&c) == INTERPRET_OK && vm.result == 123.5);
  freeChunk(&c);
  for (int i = 0; i < 257; i++) writeConstant(&c, i, 40);
  assert(c.code[510] == OP_CONSTANT && c.code[511] == 255);
  assert(c.code[512] == OP_CONSTANT_LONG);
  assert(c.code[513] == 0 && c.code[514] == 1 && c.code[515] == 0);
  for (int i = 0; i < c.count; i++) assert(getLine(&c, i) == 40);
  for (int i = 0; i < 256; i++) writeChunk(&c, OP_ADD, 40);
  writeChunk(&c, OP_RETURN, 40);
  assert(interpret(&c) == INTERPRET_OK && vm.result == 32896);
  assert(interpret(&c) == INTERPRET_OK && vm.stackCount == 0);
  freeChunk(&c);
  writeConstant(&c, 0, 50);
  writeChunk(&c, OP_NEGATE, 50);
  writeChunk(&c, OP_RETURN, 50);
  assert(interpret(&c) == INTERPRET_OK && signbit(vm.result));
  freeChunk(&c);
  const int a[] = {1, 2, -OP_MULTIPLY, 3, -OP_ADD};
  const int b[] = {1, 2, 3, -OP_MULTIPLY, -OP_ADD};
  const int d[] = {3, 2, -OP_SUBTRACT, 1, -OP_SUBTRACT};
  const int e[] = {1, 2, 3, -OP_MULTIPLY, -OP_ADD,
                   4, 5, -OP_NEGATE, -OP_DIVIDE, -OP_SUBTRACT};
  const int f[] = {4, 3, 0, 2, -OP_SUBTRACT, -OP_MULTIPLY, -OP_SUBTRACT};
  const int g[] = {4, 3, 2, -OP_NEGATE, -OP_MULTIPLY, -OP_NEGATE, -OP_ADD};
  expression(a, sizeof(a) / sizeof(a[0]), 5);
  expression(b, sizeof(b) / sizeof(b[0]), 7);
  expression(d, sizeof(d) / sizeof(d[0]), 0);
  expression(e, sizeof(e) / sizeof(e[0]), 7.8);
  expression(f, sizeof(f) / sizeof(f[0]), 10);
  expression(g, sizeof(g) / sizeof(g[0]), 10);
  freeVM();
  puts("All tests passed.");
  return 0;
}
