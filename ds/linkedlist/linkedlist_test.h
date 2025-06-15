// #pragma once
// #include <unistd.h>

// #include <cassert>
// #include <iostream>

// #include "linkedlist.h"
// void test_linkedlist(void *custom_ds) {
//   linkedlist_t *custom_ll = (linkedlist_t *)custom_ds;
//   // for (int i = 0; i < 1000; i++) {
//   //     linkedlist_insert(ll, i);
//   // }
//   // for (int i = 999; i >= 0; i--) {
//   //     assert(linkedlist_find(ll, i));
//   // }
//   // for (int i = 0; i < 100; i++) {
//   //     assert(linkedlist_remove(ll, i));
//   // }
//   const int TNODE = 10000;
//   const int DELNODE = 2000;
//   std::cout << "Inserting" << std::endl;
//   sleep(10);
//   if (SimThreadInfo::worker_machine_id == 0) {
//     for (int i = 0; i < TNODE; i += 2) {
//       assert(linkedlist_insert(custom_ll, i, i+1));
//     }
//   } else if (SimThreadInfo::worker_machine_id == 1) {
//     for (int i = 1; i < TNODE; i += 2) {
//       assert(linkedlist_insert(custom_ll, i, i+1));
//     }
//   }
//   std::cout << "Finding" << std::endl;
//   if (SimThreadInfo::worker_machine_id == 0) {
//     for (int i = 0; i < TNODE; i += 2) {
//       uint64_t val;
//       assert(linkedlist_find(custom_ll, i, &val));
//       assert(val == i+1);
//     }
//   } else if (SimThreadInfo::worker_machine_id == 1) {
//     for (int i = 1; i < TNODE; i += 2) {
//       uint64_t val;
//       assert(linkedlist_find(custom_ll, i, &val));
//       assert(val == i+1);
//     }
//   }

//   // 并发查找
//   // tbb::parallel_for(0, 1000, 1,
//   //                   [&](int i) { assert(linkedlist_find(ll, i)); });

//   // 并发删除
//   // tbb::parallel_for(0, 100, 1,
//   //                   [&](int i) { assert(linkedlist_remove(ll, i)); });
//   std::cout << "Removing" << std::endl;
//   if (SimThreadInfo::worker_machine_id == 0) {
//     for (int i = 0; i < DELNODE; i += 2) {
//       assert(linkedlist_remove(custom_ll, i));
//     }
//   } else if (SimThreadInfo::worker_machine_id == 1) {
//     for (int i = 1; i < DELNODE; i += 2) {
//       assert(linkedlist_remove(custom_ll, i));
//     }
//   }

//   if (SimThreadInfo::worker_machine_id == 0) {
//     for (int i = 0; i < DELNODE; i += 2) {
//       uint64_t val;
//       if (linkedlist_find(custom_ll, i, &val)) {
//         std::cout << i << std::endl;
//       }
//     }
//   } else if (SimThreadInfo::worker_machine_id == 1) {
//     for (int i = 1; i < DELNODE; i += 2) {
//       uint64_t val;
//       if (linkedlist_find(custom_ll, i, &val)) {
//         std::cout << i << std::endl;
//       }
//     }
//   }
//   // for (int i = 0; i < 200; i++) {
//   //     if (linkedlist_find(ll, i)) {
//   //         std::cout << i << std::endl;
//   //     }
//   // }

//   if (SimThreadInfo::worker_machine_id == 0) {
//     for (int i = DELNODE; i < TNODE; i += 2) {
//       uint64_t val;
//       assert(linkedlist_find(custom_ll, i, &val));
//     }
//   } else if (SimThreadInfo::worker_machine_id == 1) {
//     for (int i = DELNODE; i < TNODE; i += 2) {
//       uint64_t val;
//       assert(linkedlist_find(custom_ll, i, &val));
//     }
//   }

//   if (SimThreadInfo::worker_machine_id == 0) {
//     std::cout << "Machine 0: sleeping" << std::endl;
//     sleep(10);
//     for (int i = 0; i < DELNODE; i++) {
//       uint64_t val;
//       assert(!linkedlist_find(custom_ll, i, &val));
//     }
//     for (int i = DELNODE; i < TNODE; i++) {
//       uint64_t val;
//       assert(linkedlist_find(custom_ll, i, &val));
//     }
//     delete_linkedlist(custom_ll);
//   }
//   std::cout << "Test passed" << std::endl;
// }