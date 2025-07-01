#pragma once
#include "lock/lock.h"
#include "lock/pthread_rwlock.h"
#include "lock/shm_lock.h"

template <typename T>
struct LockTrait {
  using type = void;
};

template <>
struct LockTrait<ShmLock> {
  using type = soft_lock_t;
};

template <>
struct LockTrait<RWLock> {
  using type = pthread_rwlock_t;
};