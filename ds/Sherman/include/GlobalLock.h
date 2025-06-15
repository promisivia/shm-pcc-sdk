#pragma once
#include <cstdint>

#include "Common.h"
#include "utils/atomic_variable.h"

extern nt<uint64_t> *global_lock_table;
extern nt<void *> *root_ptr_list;

inline void init_global_lock_table() {
// #ifdef USE_CXL
//   global_lock_table = (nt<uint64_t> *)cacheable.malloc(define::kNumOfLock *
//                                                       sizeof(nt<uint64_t>));
// #else
  global_lock_table =
      (nt<uint64_t> *)malloc(define::kNumOfLock * sizeof(uint64_t));
// #endif
  for (auto i = 0UL; i < define::kNumOfLock; i++) {
    new (&global_lock_table[i]) nt<uint64_t>(0);
  }
}

inline void init_root_ptr() {
// #ifdef USE_CXL
//   root_ptr_list =
//       (nt<void *> *)cacheable.malloc(sizeof(void *) * define::kMaxTree);
// #else
  root_ptr_list = (nt<void *> *)malloc(sizeof(void *) * define::kMaxTree);
// #endif
  for (auto i = 0UL; i < define::kMaxTree; i++)
    new (&root_ptr_list[i]) nt<void *>(nullptr);
}
