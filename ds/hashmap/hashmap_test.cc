#include "hashmap_test.h"

#include <cassert>
#include <cstdio>

// void slave_hashmap_test(int SimThreadInfo::worker_machine_id) {
//   const int num_keys = 100;      // Number of keys to insert
//   const float self_ratio = 0.7;  // 70% self-reads

//   HashMapTest test(num_keys, self_ratio);
//   test.run_test();
// }

// int main() {
//   // Example of running the test for 3 machines
//   for (int SimThreadInfo::worker_machine_id = 0; SimThreadInfo::worker_machine_id < 3; SimThreadInfo::worker_machine_id++) {
//     slave_hashmap_test(SimThreadInfo::worker_machine_id);
//   }
//   printf("All tests passed!\n");
//   return 0;
// }
