//ch 20 challenge 3 (separate string construction cost from cached-hash lookup)
#include "bench_common.h"
int main(void) {
  const int sizes[] = {64, 4096, 65536};
  header();
  for (int shape = 0; shape < 2; shape++) {
    for (int k = 0; k < 3; k++) {
      initVM();
      int n = sizes[k];
      Table table;
      initTable(&table);
      Value* keys = ALLOCATE(Value, n);
      char (*texts)[160] = malloc((size_t)n * sizeof(*texts));
      assert(texts != NULL);
      for (int i = 0; i < n; i++) {
        if (shape) {
          memset(texts[i], 'x', 120);
          sprintf(texts[i] + 120, "%08d", i);
        } else {
          sprintf(texts[i], "%08d", i);
        }
      }
      clock_t start = clock();
      for (int i = 0; i < n; i++) {
        keys[i] = OBJ_VAL(copyString(texts[i], shape ? 128 : 8));
        tableSet(&table, keys[i], NUMBER_VAL(i));
      }
      report(shape ? "long_string_create_insert" : "short_string_create_insert",
             n, n, start, table.capacity);
      lookups(&table, keys, n, shape ? "long_string_hit" : "short_string_hit", true);
      free(texts);
      FREE_ARRAY(Value, keys, n);
      freeTable(&table);
      freeVM();
    }
  }
  return 0;
}
