#include "lock/soft_lock.h"

#include "shm/mempool.h"
#include "utils/bypass_cache.h"

SoftLock::SoftLock() {
  cacheable.clalign(memkind_pool, reinterpret_cast<void**>(&lock_),
                 sizeof(soft_lock_t));
  memset(lock_, 0, sizeof(soft_lock_t));
}

SoftLock::SoftLock(soft_lock_t* sl) : lock_(sl) {}
SoftLock::~SoftLock() { cacheable.free(memkind_pool, lock_); }

void SoftLock::shm_lock_cross_machine_bypass_cache(int id) {
  volatile uint32_t* choosing = lock_->choosing;
  volatile uint32_t* number = lock_->number;

  WRITE_NT_32(&choosing[id], static_cast<uint32_t>(true));

  memory_fence();

  int max_num = 0;
  for (int i = 0; i < NUM_CLIENTS; i++) {
    int num = READ_NT_32(&number[i]);
    if (num > max_num) {
      max_num = num;
    }
  }
  WRITE_NT_32(&number[id], max_num + 1);

  memory_fence();

  WRITE_NT_32(&choosing[id], static_cast<uint32_t>(false));

  for (int j = 0; j < NUM_CLIENTS; j++) {
    while (READ_NT_32(&choosing[j])) {
      // Busy-wait if j is choosing a number
    }

    memory_fence();

    while (READ_NT_32(&number[j]) != 0 &&
           (READ_NT_32(&number[j]) < READ_NT_32(&number[id]) ||
            (READ_NT_32(&number[j]) == READ_NT_32(&number[id]) && j < id))) {
      // Busy-wait if j has a lower number or the same number but lower id
    }
  }
}

void SoftLock::shm_unlock_cross_machine_bypass_cache(int id) {
  WRITE_NT_32(&(lock_->number[id]), 0);
}

void SoftLock::shm_lock_cross_machine(int id) {
  volatile uint32_t* choosing = lock_->choosing;
  volatile uint32_t* number = lock_->number;

  choosing[id] = true;
  memory_fence();

  int max_num = 0;
  for (int i = 0; i < NUM_CLIENTS; i++) {
    int num = number[i];
    if (num > max_num) {
      max_num = num;
    }
  }
  number[id] = max_num + 1;
  memory_fence();

  choosing[id] = false;
  // memory_fence();

  for (int j = 0; j < NUM_CLIENTS; j++) {
    while (choosing[j]) {
      // Busy-wait if j is choosing a number
    }

    // memory_fence();

    while (number[j] != 0 &&
           (number[j] < number[id] || (number[j] == number[id] && j < id))) {
      // memory_fence();
      // Busy-wait if j has a lower number or the same number but lower id
    }

    // memory_fence();
  }
}

void SoftLock::shm_unlock_cross_machine(int id) { lock_->number[id] = 0; }