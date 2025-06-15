// #pragma once

// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>

// #include "hashmap.h"

// // #define HASHMAP_SIZE 8
// #define KEY_RANGE 1000000

// void perform_inserts(int SimThreadInfo::worker_machine_id, int num_keys);
// void perform_reads(int SimThreadInfo::worker_machine_id, int num_keys, float self_ratio);

// inline int generate_key(int SimThreadInfo::worker_machine_id, int index) {
//   return (SimThreadInfo::worker_machine_id * KEY_RANGE) + index;
// }

#pragma once

#include "hashmap/hashmap.h"
// #include "linkedlist/lockfree_linkedlist.h"
#include "linkedlist/lockfree_linkedlist_sorted.h"
template <typename HashMapType>
class HashMapTest {
  // using HashMapType = _HashMapType<KeyType, ValueType, HashMapArgs...>;

 public:
  HashMapTest(int num_keys, float self_ratio, HashMapType* hashmap)
      : num_keys(num_keys), self_ratio(self_ratio), hashmap(hashmap) {}
  ~HashMapTest() = default;

  void perform_inserts() {
    for (int i = 1; i < num_keys; i++) {
      // char key[32];  // Assuming keys are strings
      // snprintf(key, sizeof(key), "machine_%d_key_%d", SimThreadInfo::worker_machine_id, i);
      hashmap->insert(i, i + 1);  // Use the key's index + 1 as the value
    }
  }
  void perform_reads() {
    int self_reads = (int)(num_keys * self_ratio);
    int other_reads = num_keys - self_reads;

    // Read keys inserted by the current machine
    for (int i = 1; i < self_reads; i++) {
      // char key[32];
      // snprintf(key, sizeof(key), "machine_%d_key_%d", SimThreadInfo::worker_machine_id, i);
      // const char* value;
      uint64_t key = i;
      uint64_t value;
      assert(hashmap->get(key, &value));
      // assert(strcmp(value, key) == 0);
      assert(value == key + 1);
    }

    // Read keys inserted by other machines
    // for (int other_machine_no = 0; other_machine_no < NUM_CLIENTS;
    //      other_machine_no++) {
    //   if (other_machine_no == SimThreadInfo::worker_machine_id) continue;
    //   for (int i = 0; i < other_reads; i++) {
    //     // char key[32];
    //     // snprintf(key, sizeof(key), "machine_%d_key_%d", other_machine_no,
    //     i);
    //     // const char* value;
    //     uint64_t key = i;
    //     uint64_t value;
    //     assert(hashmap->get(key, &value));
    //     // assert(strcmp((const char*)value, key) == 0);
    //     assert(value == key + 1);
    //   }
    // }
  }
  void run_test() {
    perform_inserts();
    perform_reads();
  }

 private:
  int SimThreadInfo::worker_machine_id;
  int num_keys;
  float self_ratio;
  HashMapType* hashmap;
};
