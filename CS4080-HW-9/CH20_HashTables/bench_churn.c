//ch 20 challenge 3 (measure tombstones and repeated replacement of live keys)
#include "bench_common.h"
int main(void) {
  initVM();
  header();
  Table old, fresh;
  initTable(&old);
  initTable(&fresh);
  for (int i = 0; i < 6144; i++) tableSet(&old, NUMBER_VAL(i), NUMBER_VAL(i));
  Value missing[2048];
  for (int i = 0; i < 2048; i++) {
    missing[i] = NUMBER_VAL(i);
    assert(tableDelete(&old, missing[i]));
  }
  tableAddAll(&old, &fresh);
  assert(old.capacity == fresh.capacity);
  //ch 20 challenge 3 (same live keys and capacity isolate the effect of tombstones)
  lookups(&old, missing, 2048, "miss_with_tombstones", false);
  lookups(&fresh, missing, 2048, "miss_after_rebuild", false);
  freeTable(&old);
  freeTable(&fresh);

  Table churn;
  initTable(&churn);
  const int live = 4096;
  const unsigned cycles = 200000;
  for (int i = 0; i < live; i++) tableSet(&churn, NUMBER_VAL(i), NUMBER_VAL(i));
  unsigned removed = 0, added = 0;
  clock_t start = clock();
  for (unsigned i = 0; i < cycles; i++) {
    removed += tableDelete(&churn, NUMBER_VAL(i));
    added += tableSet(&churn, NUMBER_VAL(i + live), NUMBER_VAL(i + live));
  }
  report("delete_insert_churn", live, cycles * 2ul, start, churn.capacity);
  assert(removed == cycles && added == cycles);
  for (unsigned i = cycles; i < cycles + live; i++) {
    Value value;
    assert(tableGet(&churn, NUMBER_VAL(i), &value));
    assert(AS_NUMBER(value) == i);
  }
  freeTable(&churn);
  freeVM();
  return 0;
}
