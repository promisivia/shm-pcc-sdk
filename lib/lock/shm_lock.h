#pragma once
#include <pthread.h>

#include <cstdint>

#include "lock/lock.h"
#include "lock/soft_lock.h"
#include "tbb/concurrent_hash_map.h"

class ShmLock : public LockBase {
 public:
  ShmLock(soft_lock_t* sl = nullptr);
  ~ShmLock();

  void lock() override;
  void unlock() override;

 protected:
  using LockMap = tbb::concurrent_hash_map<soft_lock_t*, ShmLock*>;
  using LockMapAccessor = LockMap::accessor;
  static LockMap lock_map;

  SoftLock soft_lock;
  pthread_mutex_t local_lock;
  uint64_t lock_owner;
};