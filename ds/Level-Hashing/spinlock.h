/* Spin lock using xchg
   Copied from http://locklessinc.com/articles/locks/
 */

#include "utils/atomic_variable.h"

/* Compile read-write barrier */
#define barrier() asm volatile("": : :"memory")

/* Pause instruction to prevent excess processor bus usage */
#define cpu_relax() asm volatile("pause\n": : :"memory")

static inline unsigned short xchg_8(void *ptr, unsigned char x)
{
    __asm__ __volatile__("xchgb %0,%1"
                :"=r" (x)
                :"m" (*(volatile unsigned char *)ptr), "0" (x)
                :"memory");

    return x;
}

#define BUSY 1
#ifdef NO_CC
typedef nt<uint64_t> spinlock;
#else
typedef unsigned char spinlock;
#endif

#define SPINLOCK_INITIALIZER 0

static inline void spin_lock(spinlock *lock)
{
    uint64_t expected = 0;
    while (1) {
#ifdef NO_CC
        if (lock->compare_exchange_strong(expected, BUSY, std::memory_order_acquire)) return;
        
        while (lock->load(std::memory_order_acquire) == BUSY) cpu_relax();
#else
        if (!xchg_8(lock, BUSY)) return;
    
        while (*lock) cpu_relax();
#endif
    }
}

static inline void spin_unlock(spinlock *lock)
{
#ifdef NO_CC
    lock->store(0, std::memory_order_release);
#else
    barrier();
    *lock = 0;
#endif
}

static inline int spin_trylock(spinlock *lock)
{
#ifdef NO_CC
    uint64_t expected = 0;
    return lock->compare_exchange_strong(expected, BUSY);
#else
    return xchg_8(lock, BUSY);
#endif
}
