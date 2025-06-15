#include "lock/shm_lock.h"

#include <cstdlib>

#include "shm/mempool.h"

ShmLock::LockMap ShmLock::lock_map;

ShmLock::ShmLock(soft_lock_t* sl) : soft_lock(sl) {
  LockMapAccessor a;
  bool item_found = ShmLock::lock_map.find(a, soft_lock.lock_);
  if (item_found) {
    delete a->second;
  }
  bool insert_suc = ShmLock::lock_map.insert(a, soft_lock.lock_);
  if (!insert_suc) {
    perror("Insertion failed");
  }
  a->second = this;
  /* init machine-local lock */
  local_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef LOGING
  /* init logging */
  init_logging();
#endif
}

ShmLock::~ShmLock() {
  {
    LockMapAccessor a;
    ShmLock::lock_map.find(a, soft_lock.lock_);
    free(a->second);
    ShmLock::lock_map.erase(a);
  }
  cacheable.free(memkind_pool, soft_lock.lock_);
}

void ShmLock::lock() {
  /* first acquire intra-machine lock */
  pthread_mutex_lock(&local_lock);
  /* acquire inter-machine lock */
  soft_lock.shm_lock_cross_machine_bypass_cache(SimThreadInfo::worker_machine_id);
}

void ShmLock::unlock() {
  /* first release inter-machine lock */
  soft_lock.shm_unlock_cross_machine_bypass_cache(SimThreadInfo::worker_machine_id);
  /* release intra-machine lock */
  pthread_mutex_unlock(&local_lock);
}