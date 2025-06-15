#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <vector>

#define MEMORY_SIZE (1L * 1024 * 1024 * 1024)  // 1GB
#define FLUSH_INTERVAL 2
#define CACHE_LINE_SIZE (64)

static inline void mfence() { asm volatile("mfence" ::: "memory"); }

static inline void clflush(const void* data, size_t len, bool fence = true) {
  volatile char* ptr = (char*)((unsigned long)data & (~(CACHE_LINE_SIZE - 1)));
  if (fence) mfence();
  for (; ptr < (char*)data + len; ptr += CACHE_LINE_SIZE) {
#ifdef USE_CLFLUSH
    __asm__ __volatile__("clflush %0" : "+m"(*(volatile char*)ptr));
#else
    __asm__ __volatile__(".byte 0x66; clflush %0" : "+m"(*(volatile char*)ptr));
#endif
  }
  if (fence) mfence();
}

#define REPEAT_1(x) x
#define REPEAT_2(x) REPEAT_1(x) REPEAT_1(x)
#define REPEAT_4(x) REPEAT_2(x) REPEAT_2(x)
#define REPEAT_8(x) REPEAT_4(x) REPEAT_4(x)

#define REPEAT_N_TIMES(n, x) REPEAT_##n(x)

void generate_random_addresses(std::vector<void*>& addresses, char* memory,
                               size_t num_addresses) {
  for (size_t i = 0; i < num_addresses; ++i) {
    addresses.push_back(memory + (rand() % (MEMORY_SIZE / CACHE_LINE_SIZE)) *
                                     CACHE_LINE_SIZE);
  }
}

void sequential_access(char* memory) {
  for (size_t i = 0; i < MEMORY_SIZE; i += CACHE_LINE_SIZE) {
    memory[i] = static_cast<char>(i);
  }
}

void test_clflush_with_fence(const std::vector<void*>& addresses) {
  for (size_t i = 0; i < addresses.size(); i++) {
    clflush(addresses[i], CACHE_LINE_SIZE, true);
  }
}

void test_clflush_with_partial_fence(const std::vector<void*>& addresses) {
    for (size_t i = 0; i < addresses.size(); i += 2) {
      mfence();
      clflush(addresses[i], CACHE_LINE_SIZE, false);
      clflush(addresses[i + 1], CACHE_LINE_SIZE, false);
      mfence();
    }
}

int main() {
  srand(static_cast<unsigned>(time(0)));

  // 分配1GB内存
  char* memory = new char[MEMORY_SIZE];
  std::memset(memory, 0, MEMORY_SIZE);

  // 生成随机地址列表
  std::vector<void*> addresses;
  generate_random_addresses(addresses, memory, MEMORY_SIZE / CACHE_LINE_SIZE);

  // 顺序访问内存
  sequential_access(memory);

  // 测试第一种情况：每次flush前后都使用fence
  auto start = std::chrono::high_resolution_clock::now();
  test_clflush_with_fence(addresses);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  std::cout << "Test with fence every flush: " << duration.count() << " seconds"
            << std::endl;

  // 测试第二种情况：每两次或每四次flush前后才使用fence
  start = std::chrono::high_resolution_clock::now();
  test_clflush_with_partial_fence(addresses);  // 每两次flush使用一次fence
  end = std::chrono::high_resolution_clock::now();
  duration = end - start;
  std::cout << "Test with fence every 2 flushes: " << duration.count()
            << " seconds" << std::endl;

  delete[] memory;
  return 0;
}