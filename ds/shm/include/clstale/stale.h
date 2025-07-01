#pragma once
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <atomic>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include <cstdint>

#include "utils/bypass_cache.h"
#include "utils/compiler.h"
#include "connection/establish.h"
#include "utils/sim_id.h"

struct stale_cacheline {
  void* addr;
};

struct global_process {
  size_t* ptr;
};

struct buffer_req {
  void* buffer;
  size_t size;
  size_t* head;
  size_t* tail;
  global_process* process;
};

extern global_process* g_process;

#define STALE_BUFFER_SIZE 4096 * 1024

template <typename T>
class LockFreeRingBuffer {
 public:
  explicit LockFreeRingBuffer(void* buffer, size_t size, size_t* head,
                              size_t* tail)
      : size_(size / sizeof(T)),
        buffer_((stale_cacheline*)buffer),
        head_(head),
        tail_(tail),
        process_(*tail) {
    if ((size_ & (size_ - 1)) != 0) {
      throw std::invalid_argument("Size must be a power of 2");
    }
  }

  bool push(const T& item) {
    size_t head = *head_;
    size_t next_head = (head + 1) & (size_ - 1);
    if (next_head == *tail_) {
      return false;  // Buffer is full
    }
    // CAS operation to update head_
    while (!__sync_bool_compare_and_swap(head_, head, next_head)) {
      head = *head_;
      next_head = (head + 1) & (size_ - 1);
      if (next_head == *tail_) {
        return false;  // Buffer is full
      }
    }
    buffer_[head].addr = item.addr;
    // std::cout << "push" << buffer_[head].addr << std::endl;
    return true;
  }

  //   bool pop(T& item) {
  //     size_t tail = *tail_;
  //     if (tail == head_) {
  //       return false;  // Buffer is empty
  //     }
  //     // CAS operation to update tail_
  //     while (
  //         !__sync_bool_compare_and_swap(tail_, tail, (tail + 1) & (size_ -
  //         1))) {
  //       tail = *tail_;
  //       if (tail == *head_) {
  //         return false;  // Buffer is empty
  //       }
  //     }
  //     item = buffer_[tail];
  //     return true;
  //   }

  void recycle_stale() {
    size_t min_process = 0;
    for (int i = 0; i < SimThreadInfo::worker_machine_id;) {
      __m128i g_process_cpy;
      READ_NT_128((__m128i*)&g_process[i], g_process_cpy);
      size_t proc1 = *((size_t*)&g_process_cpy);
      min_process = std::min(min_process, proc1);
      i++;
      if (i != SimThreadInfo::worker_machine_id) {
        size_t proc2 = *(((size_t*)&g_process_cpy) + 1);
        min_process = std::min(min_process, proc2);
        i++;
      }
    }
    int i = *tail_;
    while (i != min_process) {
      buffer_[i].addr = 0;
      i = (i + 1) & (size_ - 1);
    }
    *tail_ = min_process;
  }

  bool process(T& item) {
    if (process_ == *head_) {
      return false;
    }
    while (buffer_[process_].addr == 0);
    item.addr = buffer_[process_].addr;
    process_ = (process_ + 1) & (size_ - 1);

    WRITE_NT_64(&(g_process[SimThreadInfo::worker_machine_id].ptr), process_);

    return true;
  }

  const size_t size_;
  T* buffer_;
  volatile size_t* head_;
  volatile size_t* tail_;
  size_t process_;
};

extern LockFreeRingBuffer<stale_cacheline>* stale_list;

void create_stale_list(void* buffer, size_t size);

void apply_stale(void* addr);

void add_stale(void* addr);

void* process_stale(void* arg);

void* recycle_stale(void* arg);
#endif
