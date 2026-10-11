#include <stdio.h>
#include <time.h>
#include "chunk.h"
#include "vm.h"
extern VM vm;
int main(void) {
  Chunk c;
  initChunk(&c);
  initVM();
  writeConstant(&c, 1.25, 1);
  for (int i = 0; i < 65536; i++) writeChunk(&c, OP_NEGATE, 1);
  writeChunk(&c, OP_RETURN, 1);
  interpret(&c);
  clock_t start = clock();
  double checksum = 0;
  for (int i = 0; i < 1024; i++) {
    interpret(&c);
    checksum += vm.result;
  }
  printf("%.6f %.2f\n", (double)(clock() - start) / CLOCKS_PER_SEC, checksum);
  freeVM();
  freeChunk(&c);
}
