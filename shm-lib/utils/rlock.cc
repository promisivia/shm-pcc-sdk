#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include "utils/rlock.h"
#include <stdio.h>

void rlock_lock(rlock_t *l, lock_t lock, owner_t owner) {
    while (!__sync_bool_compare_and_swap(&(l->lock_), 0, owner)) {
        l->owner_ = owner;
    }
#ifdef NO_CC
    clwb((void*)&(l->lock_), sizeof(lock_t));
#endif
}

void rlock_unlock(rlock_t *l) {
    // assert (l->lock_ != 0);
    l->lock_ = 0;
    l->owner_ = 0;
#ifdef NO_CC
    clwb((void*)&(l->lock_), sizeof(lock_t));
#endif
}

void rlock_st(rlock_t *l, const lock_t value) {
    l->lock_ = value;
#ifdef NO_CC
    clwb((void*)&(l->lock_), sizeof(lock_t));
#endif
}

lock_t rlock_ld(rlock_t *l) {
#ifdef NO_CC
    clflush((void*)&(l->lock_), sizeof(lock_t));
#endif
    return l->lock_;
}

lock_t rlock_cas(rlock_t *l, lock_t expected, lock_t desired) {
    lock_t ll = __sync_val_compare_and_swap(&(l->lock_), expected, desired);
#ifdef NO_CC
    clwb((void*)&(l->lock_), sizeof(lock_t));
#endif
    return ll;
}

lock_t rlock_tas(rlock_t *l) {
    volatile lock_t* addr = &(l->lock_);
    lock_t oldval;
    __asm__ __volatile__("xchgb %0,%1"
            : "=q"(oldval), "=m"(*addr)
            : "0"((unsigned char) 0xff), "m"(*addr) : "memory");
#ifdef NO_CC
    clwb((void*)&(l->lock_), sizeof(lock_t));
#endif
    return (lock_t) oldval;
}
#endif
