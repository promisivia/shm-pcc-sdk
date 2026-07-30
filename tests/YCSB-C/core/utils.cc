#include "core/utils.h"

uint32_t VALUE_ADDR_SIZE = 0;

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE (64)
#endif

inline constexpr bool kOptReadValueByAddress = false;

namespace utils {

void AccessValueByAddress(uintptr_t addr) {
  size_t size = static_cast<size_t>(VALUE_ADDR_SIZE);
  if constexpr (kOptReadValueByAddress) {
    for (size_t i = 0; i < size; i += CACHE_LINE_SIZE) {
      // yjs: prefetch cache line
      __builtin_prefetch(reinterpret_cast<const char*>(addr) + i);
    }
  } else {
    volatile char* ptr = reinterpret_cast<volatile char*>(addr);

    for (size_t i = 0; i < size; i++) {
      (void)ptr[i];
    }
  }
}

}
