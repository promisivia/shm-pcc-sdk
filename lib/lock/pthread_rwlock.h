#pragma once
#include <pthread.h>

#include "lock/lock.h"

class RWLock : public LockBase {
 public:
  RWLock(pthread_rwlock_t* lock) : lock_(lock) {
    pthread_rwlock_init(lock_, nullptr);
  }
  void lock() { pthread_rwlock_wrlock(lock_); };
  void unlock() { pthread_rwlock_unlock(lock_); };
  void r_lock() { pthread_rwlock_rdlock(lock_); };
  void r_unlock() { pthread_rwlock_unlock(lock_); };
  ~RWLock() = default;

 private:
  pthread_rwlock_t* lock_;
};