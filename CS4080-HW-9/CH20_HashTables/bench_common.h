//ch 20 challenge 3 (share timing and validation across the benchmark programs)
#ifndef HW9_BENCH_COMMON_H
#define HW9_BENCH_COMMON_H
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "memory.h"
#include "object.h"
#include "table.h"
#include "vm.h"
#define QUERIES 2000000u
static volatile double checksum;
static void report(const char* workload, int n, unsigned long operations,
                   clock_t start, int capacity) {
  double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
  printf("%s,%d,%lu,%.9f,%.3f,%d\n", workload, n, operations, seconds,
         seconds * 1e9 / operations, capacity);
}
static void header(void) {
  puts("workload,keys,operations,seconds,ns_per_operation,capacity");
}
static void lookups(Table* table, Value* keys, int n,
                    const char* label, bool shouldFind) {
  double total = 0;
  unsigned foundCount = 0;
  clock_t start = clock();
  for (unsigned i = 0; i < QUERIES; i++) {
    unsigned index = (i * 2654435761u) % (unsigned)n;
    Value value;
    bool found = tableGet(table, keys[index], &value);
    foundCount += found;
    if (found) total += AS_NUMBER(value);
  }
  checksum = total;
  report(label, n, QUERIES, start, table->capacity);
  assert(foundCount == (shouldFind ? QUERIES : 0));
}
#endif
