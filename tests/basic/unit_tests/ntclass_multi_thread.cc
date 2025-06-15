#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <immintrin.h>

#include "utils/atomic_variable.h"

void thread_load_task(nt<int64_t>* shared_value, size_t iterations, int64_t mode) {
    auto start_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        shared_value->load();
    }
    auto end_sync = std::chrono::high_resolution_clock::now();
    auto duration_sync = std::chrono::duration_cast<std::chrono::nanoseconds>(end_sync - start_sync).count();
    std::cout << "Thread load with sync duration: " << (double)duration_sync / iterations << " nanoseconds\n";
}

void benchmark_loads(size_t iterations, size_t thread_count) {
    nt<int64_t> shared_value(0);
    std::vector<std::thread> threads;

    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_load_task, &shared_value, iterations, false);
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    auto start_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_load_task, &shared_value, iterations, 0);
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end_sync = std::chrono::high_resolution_clock::now();
    auto duration_sync = std::chrono::duration_cast<std::chrono::microseconds>(end_sync - start_sync).count();

    double lat_sync = (double)duration_sync;

    std::cout << "Thread" << thread_count << " load with sync duration: " << lat_sync << " microseconds\n";
}

int main() {
    const size_t iterations = 12800000;
    for (size_t thread_count = 1; thread_count <= 128; thread_count *= 2) {
        benchmark_loads(iterations / thread_count, thread_count);
    }
    return 0;
}
