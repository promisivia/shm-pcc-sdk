// #pragma once
// #include <cstdint>

// #include "connection/establish.h"

// #define CROSS_MACHINE_LOCK

// /* defined in establish.h */
// #define NUM_CLIENTS machine_N
// #define SimThreadInfo::worker_machine_id (SimThreadInfo::worker_machine_id)

// // Need to be 4-byte aligned
// struct soft_lock_t {
//   /* global array for software lock */
//   volatile uint32_t choosing[NUM_CLIENTS];
//   volatile uint32_t number[NUM_CLIENTS];
// };

// void init_soft_lock(soft_lock_t** sl);

// void shm_lock_cross_machine_bypass_cache(soft_lock_t* lock, int id);
// void shm_lock_cross_machine(soft_lock_t* lock, int id);

// void shm_unlock_cross_machine_bypass_cache(soft_lock_t* lock, int id);
// void shm_unlock_cross_machine(soft_lock_t* lock, int id);

#pragma once
#include <cstdint>
#include <vector>

#include "connection/establish.h"

#define CROSS_MACHINE_LOCK

struct soft_lock_t {
  /* global array for software lock */
  volatile uint32_t choosing[NUM_CLIENTS];
  volatile uint32_t number[NUM_CLIENTS];
};

class SoftLock {
 public:
  SoftLock();
  SoftLock(soft_lock_t* sl);
  ~SoftLock();

  void shm_lock_cross_machine_bypass_cache(int id);
  void shm_lock_cross_machine(int id);
  void shm_unlock_cross_machine_bypass_cache(int id);
  void shm_unlock_cross_machine(int id);

  soft_lock_t* lock_;
};