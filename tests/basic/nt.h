#pragma once
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <atomic>

// #define USE_DISAGR_CAS
#define NO_CC_USE_CLFLUSH

#define CACHE_LINE_SIZE (64)
#define mfence() asm volatile("mfence" ::: "memory")

static inline void clflush(char *data, size_t len, bool fence = true) {
  volatile char *ptr = (char *)((unsigned long)data & (~(CACHE_LINE_SIZE - 1)));
  if (fence) mfence();
  for (; ptr < data + len; ptr += CACHE_LINE_SIZE) {
#ifdef USE_CLFLUSH
    asm volatile("clflush %0" : "+m"(*(volatile char *)ptr));
#else
    asm volatile(".byte 0x66; clflush %0" : "+m"(*(volatile char *)ptr));
#endif
  }
  if (fence) mfence();
}

static inline void clwb(void *data, size_t len, bool fence = true) {
  volatile char *ptr = (char *)((unsigned long)data & (~(CACHE_LINE_SIZE - 1)));
  if (fence) mfence();
  for (; ptr < data + len; ptr += CACHE_LINE_SIZE) {
#ifdef USE_CLFLUSH
    asm volatile("clflush %0" : "+m"(*(volatile char *)ptr));
#elif USE_CLFLUSH_OPT
    asm volatile(".byte 0x66; clflush %0" : "+m"(*(volatile char *)ptr));
#elif USE_CLWB
    asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char *)(ptr)));
#endif
  }
  if (fence) mfence();
}

#define ONE_LOAD(ptr, m)             \
  do {                               \
    clwb((void *)ptr, sizeof(*ptr)); \
    return __atomic_load_n(ptr, m);  \
  } while (0)

#define ONE_STORE(ptr, val, m)       \
  do {                               \
    __atomic_store_n(ptr, val, m);   \
    clwb((void *)ptr, sizeof(*ptr)); \
  } while (0)

#ifndef USE_DISAGR_CAS
template <typename T>
class nt {
 public:
  nt(T initialValue = T()) { store(initialValue); }

  nt(const nt &other) { store(other.load()); }

  ~nt() {}

  nt<T> &operator++() {
    ++value;
    return *this;
  }

  nt<T> operator++(int) {
    nt<T> temp = *this;
    ++value;
    return temp;
  }

  nt<T> &operator=(const T &value) {
    store(value);
    return *this;
  }

  T load() const {
    clwb((void *)&value, sizeof(T));
    return __atomic_load_n(&value, memory_order_seq_cst);
  }

  void store(T newValue) {
    __atomic_store_n(&value, newValue, memory_order_seq_cst);
    clwb((void *)&value, sizeof(T));
  }

  T fetch_add(T i) {
    T old_value;
    old_value = __atomic_add_fetch(&value, i, __ATOMIC_SEQ_CST);
    helper();
    return old_value;
  }

  T fetch_sub(T i) {
    T old_value;
    old_value = __atomic_fetch_sub(&value, i, __ATOMIC_SEQ_CST);
    helper();
    return old_value;
  }

  bool compare_exchange_strong(T &expected, T desired,
                               memory_order __m = memory_order_seq_cst) {
    bool equal =
        __atomic_compare_exchange_n(&value, &expected, desired, 0, (int)__m,
                                    (int)__cmpexch_failure_order(__m));
    helper();
    return equal;
  }

 protected:
  void helper() {
#ifdef NO_CC
    clwb(&value, sizeof(T));
#endif
  }

 private:
  T value;
};
#else

template <typename T>
class nt64 {
  static_assert(sizeof(T) == sizeof(uint64_t), "T must be 64-bit type");

 public:
  nt64(void *pos, T initialValue = T()) {
    lock = 0;
    value = (T *)pos;
    store(initialValue);
  }

  nt64(nt64 &&other) noexcept {
    lock = 0;
    value = other.value;
    other.value = nullptr;
  }

  nt64 &operator=(const nt64 &other) {
    if (this != &other) {
      lock = 0;
      store(other.load());
    }
    return *this;
  }

  nt64 &operator=(nt64 &&other) noexcept {
    if (this != &other) {
      lock = 0;
      value = other.value;
      other.value = nullptr;
    }
    return *this;
  }

  nt64 &operator=(T newValue) {
    lock = 0;
    store(newValue);
    return *this;
  }

  T load() const { return *value; }

  void store(T newValue) {
    uint64_t expected = 0;
    while (!__atomic_compare_exchange_n(&lock, &expected, 1, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
    *value = newValue;
    lock = 0;
  }

  T fetch_add(T i) {
    uint64_t expected = 0;
    while (!__atomic_compare_exchange_n(&lock, &expected, 1, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
    T old_value = *value;
    *value += i;
    lock = 0;
    return old_value;
  }

  T fetch_sub(T i) {
    uint64_t expected = 0;
    while (!__atomic_compare_exchange_n(&lock, &expected, 1, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
    T old_value = *value;
    *value -= i;
    lock = 0;
    return old_value;
  }

  bool compare_exchange_strong(T &expected, T desired) {
    uint64_t lock_expected = 0;
    if (__atomic_compare_exchange_n(&lock, &lock_expected, 1, 0,
                                    __ATOMIC_SEQ_CST,
                                     __ATOMIC_SEQ_CST)) {
      if (*value == expected) {
        *value = desired;
        lock = 0;
        return true;
      }
      lock = 0;
    }
    return false;
  }

 private:
  uint64_t lock;
  T *value;
};
#endif