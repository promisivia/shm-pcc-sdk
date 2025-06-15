#include "clstale/stale.h"

#include "immintrin.h"

LockFreeRingBuffer<stale_cacheline>* stale_list;
global_process* g_process;

inline void* get_aligned_address(void* addr, std::size_t alignment = 64) {
  std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(addr);
  std::uintptr_t aligned_ptr = ptr & ~(alignment - 1);
  return reinterpret_cast<void*>(aligned_ptr);
}

void apply_stale(void* addr) { _mm_clflushopt(get_aligned_address(addr, 64)); }

void create_stale_list(void* buffer, size_t size) {
  stale_list = new LockFreeRingBuffer<stale_cacheline>(
      buffer, size, (size_t*)((uintptr_t)buffer + size),
      (size_t*)((uintptr_t)buffer + size) + 1);
  g_process = (global_process*)((uintptr_t)buffer + size) + 2;
}

void add_stale(void* addr) {
  stale_cacheline cl;
  cl.addr = addr;
  stale_list->push(cl);
}

void* process_stale(void* arg) {
  stale_cacheline cl;
  while (true) {
    if (stale_list->process(cl)) {
      apply_stale(cl.addr);
    }
  }
  return nullptr;
}

void* recycle_stale(void* arg) {
  while (true) {
    stale_list->recycle_stale();
  }
  return nullptr;
}