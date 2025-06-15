#ifndef _RLOCK_H_
#define _RLOCK_H_

#include "config.h"
#include "utils/bypass_cache.h"

typedef uint8_t lock_t;
typedef uint16_t owner_t;

#define DEFAULT_OWNER (0)

struct alignas(CACHE_LINE_SIZE) rlock {
    lock_t lock_;
    owner_t owner_;
    char padding_[CACHE_LINE_SIZE - sizeof(lock_t) - sizeof(owner_t)];
};

typedef struct rlock rlock_t;

void rlock_lock(rlock_t* l, lock_t lock, owner_t owner);
void rlock_unlock(rlock_t* l);

void rlock_st(rlock_t* l, const lock_t value);
lock_t rlock_ld(rlock_t* l);

lock_t rlock_cas(rlock_t* l, lock_t expected, lock_t desired);
lock_t rlock_tas(rlock_t* l);

#endif /* _RLOCK_H_ */