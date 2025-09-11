#include <assert.h>
#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <numa.h>
#include <numaif.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <barrier>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "utils/atomic_variable.h"
#include "utils/bypass_cache.h"
#include <unordered_set>

#define CACHELINE_SIZE 64
union CACHELINE {
  uint64_t cacheline[CACHELINE_SIZE / sizeof(uint64_t)];
  struct data {
    uint64_t value;
    union CACHELINE *next;
  } data;
};

double get_elapsed_time(struct timespec ts_begin, struct timespec ts_end) {
  struct timespec ts_elapsed;
  ts_elapsed.tv_nsec = ts_end.tv_nsec - ts_begin.tv_nsec;
  ts_elapsed.tv_sec = ts_end.tv_sec - ts_begin.tv_sec;
  if (ts_elapsed.tv_nsec < 0) {
    --ts_elapsed.tv_sec;
    ts_elapsed.tv_nsec += 1000 * 1000 * 1000;
  }
  return ts_elapsed.tv_sec + 0.000000001 * ts_elapsed.tv_nsec;
}

inline union CACHELINE *random_cas(union CACHELINE *addr) {
  int old_value = addr->data.value;
  __sync_bool_compare_and_swap(&addr->data.value, old_value, old_value + 10086);
  return addr->data.next;
}

inline union CACHELINE *random_nt_cas(union CACHELINE *addr) {
  uint64_t old_value = addr->data.value;
  nt<uint64_t> *nt_addr = (nt<uint64_t> *)&(addr->data.value);
  nt_addr->compare_exchange_strong(old_value, old_value + 10086);
  return addr->data.next;
}

inline void hit_nt_cas(union CACHELINE *addr) {
  uint64_t old_value = addr->data.value;
  nt<uint64_t> *nt_addr = (nt<uint64_t> *)&(addr->data.value);
  nt_addr->compare_exchange_strong(old_value, old_value + 10086);
}

// inline void hit_disagr_cas(union CACHELINE *addr) {
//   uint64_t old_value = addr->data.value;
//   nt<uint64_t> *nt_addr = (nt<uint64_t> *)&(addr->data.value);
//   nt_addr->compare_exchange_strong(old_value, old_value + 10086);
// }

inline void hit_write(union CACHELINE *addr) { addr->data.value = 0xDEEDD998; }

inline union CACHELINE *random_write(union CACHELINE *addr) {
  addr->data.value = 0xDEEDD998;
  return addr->data.next;
}

#define REPT4(x)                                                               \
  do {                                                                         \
    x;                                                                         \
    x;                                                                         \
    x;                                                                         \
    x;                                                                         \
  } while (0)
#define REPT16(x)                                                              \
  do {                                                                         \
    REPT4(x);                                                                  \
    REPT4(x);                                                                  \
    REPT4(x);                                                                  \
    REPT4(x);                                                                  \
  } while (0);
#define REPT64(x)                                                              \
  do {                                                                         \
    REPT16(x);                                                                 \
    REPT16(x);                                                                 \
    REPT16(x);                                                                 \
    REPT16(x);                                                                 \
  } while (0);
#define REPT256(x)                                                             \
  do {                                                                         \
    REPT64(x);                                                                 \
    REPT64(x);                                                                 \
    REPT64(x);                                                                 \
    REPT64(x);                                                                 \
  } while (0);
#define REPT1024(x)                                                            \
  do {                                                                         \
    REPT256(x);                                                                \
    REPT256(x);                                                                \
    REPT256(x);                                                                \
    REPT256(x);                                                                \
  } while (0);

#define REPEAT 1

void clflush_cache_range(void *base, size_t size) {
  const size_t cache_line_size = 64;
  uintptr_t ptr = (uintptr_t)base;

  for (size_t i = 0; i < size; i += cache_line_size) {
    asm volatile("clflush (%0)" : : "r"(ptr + i) : "memory");
  }

  asm volatile("mfence" : : : "memory");
}

void nt_cas_operation(std::vector<double> &times, int thread_id,
                      volatile uint64_t *target, int rept) {
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  int rept_time = rept / 1024;
  uint64_t old_value = *target;
  nt<uint64_t> *nt_addr = (nt<uint64_t> *)(target);
  for (int i = 0; i < rept_time; ++i) {
    REPT1024(nt_addr->compare_exchange_strong(old_value, old_value + 10086));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  times[thread_id] = elapsed;
}

void cas_operation(std::vector<double> &times, int thread_id,
                   volatile uint64_t *target, int rept) {
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  int rept_time = rept / 1024;
  uint64_t old_value = *target;
  std::atomic<uint64_t> *nt_addr = (std::atomic<uint64_t> *)(target);
  for (int i = 0; i < rept_time; ++i) {
    REPT1024(nt_addr->compare_exchange_strong(old_value, old_value + 10086));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  times[thread_id] = elapsed;
}

void read_operation(std::vector<double> &times, int thread_id,
                    volatile uint64_t *target, uint64_t rept) {
  struct timespec ts_begin, ts_end;
  uint64_t rept_time = rept / 1024;
  volatile int value;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (uint64_t i = 0; i < rept_time; ++i) {
    REPT1024(asm volatile("" : "=r"(value) : "r"(*target) :));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  times[thread_id] = elapsed;
}

void read_flush_operation(std::vector<double> &times, int thread_id,
                          volatile uint64_t *target, uint64_t rept) {
  struct timespec ts_begin, ts_end;
  uint64_t rept_time = rept / 1024;
  volatile int value;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (uint64_t i = 0; i < rept_time; ++i) {
    REPT1024(clflush_cache_range((void *)target, 64);
             asm volatile("" : "=r"(value) : "r"(*target) : "memory"));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  times[thread_id] = elapsed;
}

void write_operation(double &times, uint64_t *target, uint64_t rept) {
  struct timespec ts_begin, ts_end;
  uint64_t rept_time = rept / 1024;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (uint64_t i = 0; i < rept_time; ++i) {
    REPT1024(*target = 1);
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  times = elapsed;
}

void write_flush_operation(double &times, uint64_t *target, int rept) {
  struct timespec ts_begin, ts_end;
  int rept_time = rept / 1024;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (int i = 0; i < rept_time; ++i) {
    REPT1024(clflush_cache_range(target, 64); *target = 1);
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  times = elapsed;
}

void write_clwb_operation(std::vector<double> &times, int thread_id,
                          uint64_t *target, int rept) {
  struct timespec ts_begin, ts_end;
  int rept_time = rept / 1024;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (int i = 0; i < rept_time; ++i) {
    REPT1024(*target = 1; clwb(target, sizeof(uint64_t)));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  times[thread_id] = elapsed;
}

void die(const char *msg) {
  printf("%s, %s\n", msg, strerror(errno));
  exit(1);
}

#define PAGE_SIZE (sysconf(_SC_PAGESIZE))
#define PAGE_MASK (~(PAGE_SIZE - 1))

void *get_uncached_mem(const char *dev, int size, int numa_node = -1) {
  assert(PAGE_SIZE != -1);

  int fd = open(dev, O_RDWR, 0);
  if (fd == -1)
    die("couldn't open device");

  printf("mmap()'ing %s\n", dev);

  if (size & ~PAGE_MASK)
    size = (size & PAGE_MASK) + PAGE_SIZE;
  if (numa_node != -1)
    assert(ioctl(fd, 0, numa_node) == 0);

  void *map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED)
    die("mmap failed.");
  printf("mmap() returned %p\n", map);
  return map;
}

enum ALLOC_TYPE {
  UNCACHED_MEM_TEST = 0,
  ALLOC_NUMA_TEST = 1,
  MMAP_NUMA_TEST = 2,
  INVALID_ALLOC_TYPE,
};

enum CACHE_TYPE {
  CACHED = 0,
  UNCACHED = 1,
  UNCACHED_FLUSH = 2,
  INVALID_CACHE_TYPE,
};

CACHE_TYPE cache_type;
ALLOC_TYPE alloc_type;
size_t map_size = 1024ul * 1024 * 1024; // 默认大小
const char *dev_name = "/dev/uncached_mem_dev";
int ram_numa_node = 4;

void *allocate_memory(size_t size) {
  void *addr;

  switch (alloc_type) {
  case UNCACHED_MEM_TEST:
    addr = get_uncached_mem(dev_name, size, ram_numa_node);
    break;
  case ALLOC_NUMA_TEST:
    if (ram_numa_node > 0 && ram_numa_node < numa_max_node()) {
      addr = numa_alloc_onnode(size, ram_numa_node);
      unsigned long nodemask = 1 << ram_numa_node;
      if (mbind(addr, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) !=
          0) {
        perror("mbind failed");
        exit(1);
      }
    } else {
      addr = malloc(size);
    }
    break;
  case MMAP_NUMA_TEST:
    if (ram_numa_node > 0 && ram_numa_node < numa_max_node()) {
      addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      unsigned long nodemask = 1 << ram_numa_node;
      if (mbind(addr, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) !=
          0) {
        perror("mbind failed");
        exit(1);
      }
    } else {
      addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    }
    break;
  default:
    fprintf(stderr, "Unknown memory allocation method: %d\n", alloc_type);
    exit(1);
  }
  return addr;
}

void read_miss_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(map_size);
  size_t *offsets =
      (size_t *)malloc(map_size / CACHELINE_SIZE * sizeof(size_t));
  for (size_t i = 0; i < map_size / CACHELINE_SIZE; ++i)
    offsets[i] = i;
  srand(time(NULL));
  for (size_t i = 1; i < map_size / CACHELINE_SIZE; ++i) {
    size_t j = 1 + rand() % (map_size / CACHELINE_SIZE - 1);
    size_t temp = offsets[i];
    offsets[i] = offsets[j];
    offsets[j] = temp;
  }

  for (size_t i = 0; i < map_size / CACHELINE_SIZE - 1; ++i) {
    imap[offsets[i]].data.next = imap + offsets[i + 1];
  }
  imap[offsets[map_size / CACHELINE_SIZE - 1]].data.next = imap;

  clflush_cache_range(imap, map_size);

  volatile union CACHELINE *p = imap;
  size_t re_time = REPEAT * tsize / 1024;

  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (size_t i = 0; i < re_time; ++i) {
    REPT1024(p = p->data.next);
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("cache miss test: %fsec, %fns/load\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

void read_hit_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  unsigned int sum = 0;
  size_t re_time = REPEAT * tsize / 1024;
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (size_t i = 0; i < re_time; i++) {
    REPT1024(sum = imap[0].data.value);
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  double elapsed = get_elapsed_time(ts_begin, ts_end);

  printf("cache hit took %fsec. %fns/load\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

void write_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(map_size);
  unsigned int value = 0xDEADBEEF; // 初始化写入值
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (int repeat = 0; repeat < REPEAT; repeat++) {
    for (size_t i = 0; i < tsize; i++) {
      imap[i].data.value = value;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("write test: %fsec, %fns/load\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

void cas_miss_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(map_size);
  size_t *offsets =
      (size_t *)malloc(map_size / CACHELINE_SIZE * sizeof(size_t));
  for (size_t i = 0; i < map_size / CACHELINE_SIZE; ++i)
    offsets[i] = i;
  srand(time(NULL));
  for (size_t i = 1; i < map_size / CACHELINE_SIZE; ++i) {
    size_t j = 1 + rand() % (map_size / CACHELINE_SIZE - 1);
    size_t temp = offsets[i];
    offsets[i] = offsets[j];
    offsets[j] = temp;
  }

  for (size_t i = 0; i < map_size / CACHELINE_SIZE - 1; ++i) {
    imap[offsets[i]].data.next = imap + offsets[i + 1];
  }
  imap[offsets[map_size / CACHELINE_SIZE - 1]].data.next = imap;

  clflush_cache_range(imap, map_size);

  union CACHELINE *p = imap;
  size_t re_time = tsize / 1024;

  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (size_t j = 0; j < re_time; ++j) {
    REPT1024(p = random_cas(p));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("cas miss test: %fsec, %fns/load\n", elapsed,
         elapsed / (tsize) * (1000 * 1000 * 1000));
}

void cas_hit_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  size_t re_time = REPEAT * tsize / 1024;
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (size_t i = 0; i < re_time; i++) {
    REPT1024(random_cas(imap));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  double elapsed = get_elapsed_time(ts_begin, ts_end);

  printf("cas hit test: %fsec. %fns/load\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

void store_miss_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(map_size);
  size_t *offsets =
      (size_t *)malloc(map_size / CACHELINE_SIZE * sizeof(size_t));
  for (size_t i = 0; i < map_size / CACHELINE_SIZE; ++i)
    offsets[i] = i;
  srand(time(NULL));
  for (size_t i = 1; i < map_size / CACHELINE_SIZE; ++i) {
    size_t j = 1 + rand() % (map_size / CACHELINE_SIZE - 1);
    size_t temp = offsets[i];
    offsets[i] = offsets[j];
    offsets[j] = temp;
  }

  for (size_t i = 0; i < map_size / CACHELINE_SIZE - 1; ++i) {
    imap[offsets[i]].data.next = imap + offsets[i + 1];
  }
  imap[offsets[map_size / CACHELINE_SIZE - 1]].data.next = imap;

  clflush_cache_range(imap, map_size);

  union CACHELINE *p = imap;
  size_t re_time = tsize / 1024;

  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (size_t j = 0; j < re_time; ++j) {
    REPT1024(p = random_write(p));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("store miss test: %fsec, %fns/load\n", elapsed,
         elapsed / (tsize) * (1000 * 1000 * 1000));
}

void store_hit_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  size_t re_time = REPEAT * tsize / 1024;
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (size_t i = 0; i < re_time; i++) {
    REPT1024(hit_write(imap));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  double elapsed = get_elapsed_time(ts_begin, ts_end);

  printf("store hit test: %fsec. %fns/load\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

void nt_cas_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  size_t re_time = REPEAT * tsize / 1024;
  union CACHELINE *a = &imap[0];
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (size_t i = 0; i < re_time; i++) {
    REPT1024(hit_nt_cas(a));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("sim cas test: %fsec, %fns/load\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

#ifdef USE_DISAGR_CAS
void nt_cas_load_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  size_t re_time = REPEAT * tsize / 1024;
  nt<uint64_t> a(&imap[0].data.value, 0);
  struct timespec ts_begin, ts_end;
  uint64_t tmp;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (int i = 0; i < re_time; i++) {
    REPT1024(tmp += a.load());
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("nt cas load test: %fsec, %fns/load\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

void disagr_cas_load_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  initialize_memkind_fixed(0);
  size_t re_time = REPEAT * tsize / 1024;
  nt<uint64_t> *a = new nt<uint64_t>(0);
  struct timespec ts_begin, ts_end;
  uint64_t tmp;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (int i = 0; i < re_time; i++) {
    REPT1024(tmp += a->load());
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("disaggregated cas load test: %fsec, %fns/load\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

void disagr_cas_store_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  initialize_memkind_fixed(0);
  size_t re_time = REPEAT * tsize / 1024;
  nt<uint64_t> *a = new nt<uint64_t>(0);
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (int i = 0; i < re_time; i++) {
    REPT1024(a->store(i));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("disaggregated cas store test: %fsec, %fns/store\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}

void disagr_cas_test(size_t tsize) {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  initialize_memkind_fixed(0);
  size_t re_time = REPEAT * tsize / 1024;
  uint64_t expect = 0xDEADBBBB;
  nt<uint64_t> *a = new nt<uint64_t>(0);
  a->store(expect);
  struct timespec ts_begin, ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  for (int i = 0; i < re_time; i++) {
    REPT1024(a->compare_exchange_strong(expect, expect));
  }
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double elapsed = get_elapsed_time(ts_begin, ts_end);
  printf("disaggregated cas test: %fsec, %fns/store\n", elapsed,
         elapsed / (REPEAT * tsize) * (1000 * 1000 * 1000));
}
#endif

void bind_thread_to_cpu(pthread_t thread, int cpu_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id, &cpuset);
  int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
  }
}

void para_read_test() {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  std::vector<uint32_t> thread_num_array = {1, 2, 4, 8, 16, 32, 64, 96};
  uint64_t total_rept = 1024ULL * 1024;
  if (cache_type == CACHED) {
    total_rept = 1024ULL * 1024 * 1024 * 8;
  }
  int max_cpus = std::thread::hardware_concurrency();
  std::cout << "Max CPUs: " << max_cpus << std::endl;
  std::cout << "Total rept: " << total_rept << std::endl;
  for (auto thread_num : thread_num_array) {
    std::vector<double> times(thread_num, 0.0);
    std::vector<std::thread> threads(thread_num);
    std::barrier<> sync_point(thread_num);
    std::cout << "Average read time for " << thread_num << " threads"
              << std::endl;
    for (uint32_t i = 0; i < thread_num; ++i) {
      if (cache_type == UNCACHED_FLUSH)
        threads[i] = std::thread([&, i]() {
          sync_point.arrive_and_wait();
          read_flush_operation(times, i, &imap[0].data.value, total_rept);
        });
      else
        threads[i] = std::thread([&, i]() {
          sync_point.arrive_and_wait();
          read_operation(times, i, &imap[0].data.value, total_rept);
        });
      bind_thread_to_cpu(threads[i].native_handle(), i % max_cpus);
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
      threads[i].join();
    }
    double average_time = 0.0;
    for (uint32_t i = 0; i < thread_num; ++i) {
      if (cache_type != CACHED) {
        std::cout << "Average time: " << times[i] * 1e9 / total_rept << "ns"
                  << std::endl;
      }
      average_time += times[i];
    }
    average_time /= thread_num;

    // Thoughput
    double max_time = *max_element(times.begin(), times.end());
    // std::cout << "Max time: " << max_time << "ns" << "Average time: " << average_time << "ns" << std::endl;
    double thoughput = (double)(thread_num * total_rept) / (double)(max_time);
    double average_throughput = (double)(thread_num * total_rept) / (double)(average_time);
    std::cout << "Average throughput: " << thoughput << "ops/s" << std::endl;
    // std::cout << "Average throughput: " << average_throughput << "ops/s" << std::endl;
  }
}

void para_read_different_mem_test() {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE * 192);
  std::vector<uint32_t> thread_num_array = {1, 2, 4, 8, 16, 32, 64, 96};
  uint64_t total_rept = 1024ULL * 1024 * 8;
  if (cache_type == CACHED) {
    total_rept = 1024ULL * 1024 * 1024 * 8;
  }
  int max_cpus = std::thread::hardware_concurrency();
  for (auto thread_num : thread_num_array) {
    std::vector<double> times(thread_num, 0.0);
    std::vector<std::thread> threads(thread_num);
    std::barrier<> sync_point(thread_num);
    std::cout << "Average read time for " << thread_num << " threads, total rept: " 
        << total_rept << std::endl;
    for (uint32_t i = 0; i < thread_num; ++i) {
      if (cache_type == UNCACHED_FLUSH)
        threads[i] = std::thread([&, i]() {
          // bind_thread_to_cpu(threads[i].native_handle(), i % (max_cpus / 2) + (i % 2) * (max_cpus / 2));
          sync_point.arrive_and_wait();
          read_flush_operation(times, i, &imap[i].data.value, total_rept);
        });
      else
        threads[i] = std::thread([&, i]() {
          sync_point.arrive_and_wait();
          read_operation(times, i, &imap[i].data.value, total_rept);
        });
      bind_thread_to_cpu(threads[i].native_handle(), i % max_cpus);
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
      threads[i].join();
    }
    double average_time = 0.0;
    for (uint32_t i = 0; i < thread_num; ++i) {
      // std::cout << "Average time: " << times[i] * 1e9 / total_rept << "ns"
      //           << std::endl;
      average_time += times[i];
    }
    average_time /= thread_num;

    // Thoughput
    double max_time = *max_element(times.begin(), times.end());
    double thoughput = (double)(thread_num * total_rept) / (double)(max_time);
    std::cout << "Average throughput: " << thoughput << "ops/s" << std::endl;
  }
}

void para_write_test() {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  std::vector<uint32_t> thread_num_array = {1, 2, 4, 8, 16, 32, 64, 96};
  size_t total_rept = 1024 * 1024 * 16;
  int max_cpus = std::thread::hardware_concurrency();
  for (auto thread_num : thread_num_array) {
    std::vector<double> times(thread_num, 0.0);
    std::vector<std::thread> threads(thread_num);
    std::barrier sync_point(thread_num);
    std::cout << "Average write time for " << thread_num << " threads"
              << std::endl;
    for (uint32_t i = 0; i < thread_num; ++i) {
      if (cache_type == UNCACHED_FLUSH)
        threads[i] = std::thread([&, i]() {
          sync_point.arrive_and_wait();
          write_flush_operation(times[i], &imap[0].data.value, total_rept);
        });
      else
        threads[i] = std::thread([&, i]() {
          sync_point.arrive_and_wait();
          write_operation(times[i], &imap[0].data.value, total_rept);
        });
      bind_thread_to_cpu(threads[i].native_handle(), i % max_cpus);
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
      threads[i].join();
    }
    double average_time = 0.0;
    for (uint32_t i = 0; i < thread_num; ++i) {
      average_time += times[i];
    }
    average_time /= thread_num;

    // Throughput
    double max_time = *max_element(times.begin(), times.end());
    double thoughput = (double)(thread_num * total_rept) / (double)(max_time);
    std::cout << "Average throughput: " << thoughput << "ops/s" << std::endl;
  }
}

template <decltype(nt_cas_operation) Func> void para_cas_test_tmpl() {
  union CACHELINE *imap = (union CACHELINE *)allocate_memory(CACHELINE_SIZE);
  std::vector<uint32_t> thread_num_array = {1, 2, 4, 8, 16, 32, 64};
  size_t total_rept = 1024 * 1024 * 1;
  int max_cpus = std::thread::hardware_concurrency();
  for (auto thread_num : thread_num_array) {
    std::vector<double> times(thread_num, 0.0);
    std::vector<std::thread> threads(thread_num);
    std::barrier<> sync_point(thread_num);
    std::cout << "Average CAS time for " << thread_num << " threads"
              << std::endl;
    for (uint32_t i = 0; i < thread_num; ++i) {
      threads[i] = std::thread([&, i]() {
        sync_point.arrive_and_wait();
        Func(times, i, &imap[0].data.value, total_rept);
      });
      bind_thread_to_cpu(threads[i].native_handle(), i % max_cpus);
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
      threads[i].join();
    }
    double average_time = 0.0;

    for (uint32_t i = 0; i < thread_num; ++i) {
      // std::cout << "Average time for thread " << i << " : "
      //           << times[i] * 1e9 / total_rept << "ns" << std::endl;
      average_time += times[i];
    }
    average_time /= thread_num;
    std::cout << "Average time: " << average_time * 1e9 / total_rept << "ns"
              << std::endl;

    // Thoughput
    double max_time = *max_element(times.begin(), times.end());
    double thoughput = (double)(thread_num * total_rept) / (double)(max_time);
    std::cout << "Average throughput: " << thoughput << "ops/s" << std::endl;
  }
}

void para_nt_cas_test() { para_cas_test_tmpl<nt_cas_operation>(); }

void para_cas_test() { para_cas_test_tmpl<cas_operation>(); }

void bank_read_test() {
  union CACHELINE *imap =
      (union CACHELINE *)allocate_memory(CACHELINE_SIZE * (1 << 15));
  uint32_t thread_num = 2;
  size_t total_rept = 1024 * 1000;
  uint32_t max_cpus = std::thread::hardware_concurrency();

  std::vector<uint32_t> itl_vector{0};
  // for (uint32_t itl = 1024; itl <= 1024; itl++) {
  //   itl_vector.push_back(itl);
  // }
  for (int itl = 0; itl < 8; itl++) {
    std::cout << "Average read time for " << thread_num << " threads"
              << std::endl;
    std::vector<double> times(thread_num, 0.0);
    std::vector<std::thread> threads(thread_num);
    std::barrier<> sync_point(thread_num);
    // for (uint32_t i = 0; i < 1; ++i) {
    //   if (cache_type == UNCACHED_FLUSH)
    //     threads[i] = std::thread([&, i]() {
    //       sync_point.arrive_and_wait();
    //       read_flush_operation(times, i, &imap[i * itl].data.value,
    //       total_rept);
    //     });
    //   else
    //     threads[i] = std::thread([&, i]() {
    //       sync_point.arrive_and_wait();
    //       read_operation(times, i, &imap[i * itl].data.value, total_rept);
    //     });
    //   bind_thread_to_cpu(threads[i].native_handle(), i % max_cpus);
    // }
    {
      threads[0] = std::thread([&]() {
        sync_point.arrive_and_wait();
        read_operation(times, 0, &imap[0].cacheline[0], total_rept);
      });
      bind_thread_to_cpu(threads[0].native_handle(), 0 % max_cpus);
    }
    {
      threads[1] = std::thread([&]() {
        sync_point.arrive_and_wait();
        read_operation(times, 1, &imap[1].cacheline[itl], total_rept);
      });
      bind_thread_to_cpu(threads[1].native_handle(), 1 % max_cpus);
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
      threads[i].join();
    }
    double average_time = 0.0;
    for (uint32_t i = 0; i < thread_num; ++i) {
      auto tmp_time = times[i] * 1e9 / total_rept;
      if (tmp_time < 160)
        break;
      std::cout << "Average time: " << tmp_time << "ns, interleave " << itl
                << std::endl;
      average_time += times[i];
    }
    average_time /= thread_num;
  }
}

void usage(int ac, char **av) {
  printf("Usage: %s <cache_type> <alloc_type> [alloc_option] <test_name>\n",
         av[0]);
  printf("\nCache Types:\n");
  printf("  cached\n");
  printf("  uncached\n");
  printf("  uncached-flush\n");
  printf("\nAllocation Types & Options (alloc_option is optional):\n");
  printf("  uncached_mem [device_name]   (default: %s)\n", dev_name);
  printf("  alloc_numa [numa_node]       (default: %d)\n", ram_numa_node);
  printf("  mmap_numa [numa_node]        (default: %d)\n", ram_numa_node);
  printf("\nTest Names:\n");
  printf("  read_miss, read_hit, write, cas_miss, cas_hit, store_miss, "
         "store_hit, nt_cas, para_read, para_write, bank_read");
#ifdef USE_DISAGR_CAS
  printf(",\n  nt_cas_load, disagr_cas_load, disagr_cas_store, disagr_cas");
#endif
  printf("\n");
  exit(1);
}

int parse_argument(int argc, char **argv) {
  if (argc < 4) { // program, cache_type, alloc_type, test_name (minimum)
    usage(argc, argv);
  }

  // 解析 cache_type
  if (!strcmp(argv[1], "cached"))
    cache_type = CACHED;
  else if (!strcmp(argv[1], "uncached"))
    cache_type = UNCACHED;
  else if (!strcmp(argv[1], "uncached-flush"))
    cache_type = UNCACHED_FLUSH;
  else {
    printf("Invalid cache_type: %s\n", argv[1]);
    usage(argc, argv);
  }

  int test_name_index = -1;

  // 解析 alloc_type 和相关选项
  const char *current_alloc_type_str = argv[2];

  if (!strcmp(current_alloc_type_str, "uncached_mem")) {
    alloc_type = UNCACHED_MEM_TEST;
  } else if (!strcmp(current_alloc_type_str, "alloc_numa")) {
    alloc_type = ALLOC_NUMA_TEST;
  } else if (!strcmp(current_alloc_type_str, "mmap_numa")) {
    alloc_type = MMAP_NUMA_TEST;
  } else {
    printf("Invalid alloc_type: %s\n", argv[2]);
    usage(argc, argv);
  }

  switch (alloc_type) {
  case UNCACHED_MEM_TEST:
    map_size = 1 << 22; // Specific map_size for this alloc_type
    if (argc ==
        4) { // Case: prog cache uncached_mem test_name (dev_name uses default)
      test_name_index = 3;
    } else { // Case: argc >= 5, prog cache uncached_mem dev_name test_name ...
      dev_name = argv[3]; // User-provided device name
      test_name_index = 4;
    }
    break;
  case ALLOC_NUMA_TEST:
  case MMAP_NUMA_TEST:
    if (argc == 4) { // Case: prog cache mmap_numa test_name (ram_numa_node uses
                     // default)
      test_name_index = 3;
    } else { // Case: argc >= 5, prog cache mmap_numa numa_node test_name ...
      ram_numa_node = atoi(argv[3]); // User-provided NUMA node
      test_name_index = 4;
    }
    break;
  default:
    printf("Invalid alloc_type: %s\n", argv[2]);
    usage(argc, argv);
  }

  // 检查测试名称是否存在
  if (test_name_index == -1 || test_name_index >= argc) {
    printf("Missing or invalid test name argument.\n");
    usage(argc, argv);
  }

  return test_name_index; // 返回测试名称的索引
}

struct TestEntry {
  void (*func_ptr_tsize)(size_t);
  void (*func_ptr_void)();
};

int main(int ac, char **av) {
  // 解析命令行参数
  int test_name_idx = parse_argument(ac, av);
  auto test_name = std::string(av[test_name_idx]);

  // 打印配置信息
  printf("Configuration:\n");
  printf("  Cache Type: %s\n", av[1]);
  printf("  Alloc Type: %s\n", av[2]);
  if (alloc_type == UNCACHED_MEM_TEST) {
    printf("  Device Name: %s\n", dev_name);
  } else if (alloc_type == ALLOC_NUMA_TEST || alloc_type == MMAP_NUMA_TEST) {
    printf("  NUMA Node: %d\n", ram_numa_node);
  }
  printf("  Test Name: %s\n", test_name.c_str());
  printf("----------------------------------------\n");

  int tsize = map_size / sizeof(int); // 注意：tsize 可能需要根据测试调整
  pthread_t this_thread = pthread_self();
  bind_thread_to_cpu(this_thread, 0); // 将主线程绑定到 CPU 0

  std::unordered_map<std::string, TestEntry> test_table = {
      {"read_miss", {read_miss_test, nullptr}},
      {"read_hit", {read_hit_test, nullptr}},
      {"write", {write_test, nullptr}},
      {"cas_miss", {cas_miss_test, nullptr}},
      {"cas_hit", {cas_hit_test, nullptr}},
      {"store_miss", {store_miss_test, nullptr}},
      {"store_hit", {store_hit_test, nullptr}},
      {"nt_cas", {nt_cas_test, nullptr}},
#ifdef USE_DISAGR_CAS
      {"nt_cas_load", {nt_cas_load_test, nullptr}},
      {"disagr_cas_load", {disagr_cas_load_test, nullptr}},
      {"disagr_cas_store", {disagr_cas_store_test, nullptr}},
      {"disagr_cas", {disagr_cas_test, nullptr}},
#endif
      {"para_read", {nullptr, para_read_test}},
      {"para_read_different_mem", {nullptr, para_read_different_mem_test}},
      {"para_write", {nullptr, para_write_test}},
      {"para_nt_cas", {nullptr, para_nt_cas_test}},
      {"para_cas", {nullptr, para_cas_test}},
      {"bank_read", {nullptr, bank_read_test}}};

  bool test_found = false;

  if (test_table.find(test_name) != test_table.end()) {
    auto entry = test_table[test_name];
    if (entry.func_ptr_tsize) {
      entry.func_ptr_tsize(tsize);
    } else if (entry.func_ptr_void) {
      entry.func_ptr_void();
    }
    test_found = true;
  }

  if (!test_found) {
    printf("Unknown test name: %s\n", test_name.c_str());
    usage(ac, av);
  }

  return 0;
}
