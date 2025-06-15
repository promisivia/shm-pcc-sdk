#ifndef __HOT__ROWEX__SPIN_LOCK__
#define __HOT__ROWEX__SPIN_LOCK__

#include <atomic>

#include "utils/atomic_variable.h"
#include "utils/config.h"

namespace hot { namespace rowex {

#ifdef NO_CC
class SpinLock {
  nt<uint32_t> mFlag;

 public:
  SpinLock() : mFlag(0) {}

  void lock() {
	uint32_t expected = 0;
    while (mFlag.compare_exchange_strong(expected, 1) == false) {
      _mm_pause();
    }
  }

  void unlock() { mFlag.store(0); }
};
#else
class SpinLock {
	std::atomic_flag mFlag;

public:
	SpinLock() : mFlag(ATOMIC_FLAG_INIT) {
	}

	void lock() {
		bool wasSetBefore;
		while((wasSetBefore = mFlag.test_and_set())) {
			_mm_pause();
		}
	}

	void unlock() {
		mFlag.clear(std::memory_order_release);
	}

};
#endif

}}

#endif