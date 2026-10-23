//ch 20 challenge 3 (compare numeric hits, misses, key patterns, and table sizes)
#include "bench_common.h"
int main(void) {
  const int sizes[] = {64, 4096, 65536};
  initVM();
  header();
  for (int shape = 0; shape < 2; shape++) {
    for (int k = 0; k < 3; k++) {
      int n = sizes[k];
      double stride = shape == 0 ? 1 : 65536;
      Value* keys = ALLOCATE(Value, n);
      Value* misses = ALLOCATE(Value, n);
      for (int i = 0; i < n; i++) {
        keys[i] = NUMBER_VAL(i * stride);
        misses[i] = NUMBER_VAL((i + n) * stride);
      }
      Table table;
      initTable(&table);
      clock_t start = clock();
      for (int i = 0; i < n; i++) tableSet(&table, keys[i], NUMBER_VAL(i));
      report(shape ? "strided_insert_grow" : "sequential_insert_grow",
             n, n, start, table.capacity);
      lookups(&table, keys, n, shape ? "strided_hit" : "sequential_hit", true);
      lookups(&table, misses, n, shape ? "strided_miss" : "sequential_miss", false);
      freeTable(&table);
      FREE_ARRAY(Value, keys, n);
      FREE_ARRAY(Value, misses, n);
    }
  }
  freeVM();
  return 0;
}
