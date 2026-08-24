#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

struct alignas(64) AlignedAtomic {
  std::atomic<uint64_t> v;
};

static inline void do_clflush_mfence(void* p) {
  asm volatile("clflush (%0)" : : "r"(p) : "memory");
  asm volatile("mfence" : : : "memory");
}

void worker(AlignedAtomic* shared, uint64_t iters,
            std::atomic<uint64_t>* issued_ops) {
  for (uint64_t i = 0; i < iters; ++i) {
    uint64_t expected = shared->v.load(std::memory_order_relaxed);
    uint64_t desired = expected + 1;
    (void)shared->v.compare_exchange_weak(expected, desired,
                                          std::memory_order_seq_cst,
                                          std::memory_order_seq_cst);
    do_clflush_mfence((void*)&shared->v);
    issued_ops->fetch_add(1, std::memory_order_relaxed);
  }
}

int main(int argc, char** argv) {
  uint64_t iters_per_thread = 100000;
  uint64_t threads = std::thread::hardware_concurrency();
  if (threads == 0) threads = 8;

  if (argc >= 2) threads = std::strtoull(argv[1], nullptr, 10);
  if (argc >= 3) iters_per_thread = std::strtoull(argv[2], nullptr, 10);

  AlignedAtomic shared;
  shared.v.store(0, std::memory_order_relaxed);

  std::atomic<uint64_t> issued_ops(0);
  std::vector<std::thread> workers;
  workers.reserve(threads);

  auto start = std::chrono::steady_clock::now();
  for (uint64_t t = 0; t < threads; ++t) {
    workers.emplace_back(worker, &shared, iters_per_thread, &issued_ops);
  }
  for (auto& th : workers) th.join();
  auto end = std::chrono::steady_clock::now();

  const double sec = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
  const uint64_t total_ops = issued_ops.load(std::memory_order_relaxed);

  std::cout << "CAS+clflush+mfence benchmark\n";
  std::cout << "threads=" << threads << ", iterations_per_thread=" << iters_per_thread << "\n";
  std::cout << "total_issued_ops=" << total_ops << "\n";
  std::cout << "final_value=" << shared.v.load(std::memory_order_relaxed) << "\n";
  std::cout << "elapsed_sec=" << sec << "\n";
  std::cout << "throughput_ops_per_sec=" << (total_ops / sec) << "\n";
  std::cout << "avg_ns_per_op=" << (sec * 1e9 / total_ops) << "\n";
  return 0;
}
