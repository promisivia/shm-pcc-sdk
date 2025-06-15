#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <immintrin.h>

#include "../nt.h"

template <typename T>
T load_with_sync(std::atomic<T>* ptr) {
    clwb(ptr, sizeof(T));
    T value = ptr->load(std::memory_order_seq_cst);
    // memory_fence();
    return value;
}

template <typename T>
T nt_load(std::atomic<T>* ptr) {
    T value = READ_NT_64(ptr);
    // memory_fence();
    return value;
}

template <typename T>
T load_without_sync(std::atomic<T>* ptr) {
    T value = ptr->load(std::memory_order_seq_cst);
    // memory_fence();
    return value;
}

// 每个线程的加载任务
void thread_load_task(std::atomic<int64_t>* shared_value, size_t iterations, int64_t mode) {
    for (size_t i = 0; i < iterations; ++i) {
        if (mode == 0) {
            load_with_sync(shared_value);
        } else if (mode == 1) {
            load_without_sync(shared_value);
        } else if (mode == 2) {
            nt_load(shared_value);
        }
    }
}

void benchmark_loads(size_t iterations, size_t thread_count) {
    std::atomic<int64_t> shared_value(0);
    std::vector<std::thread> threads;

    // 预热
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_load_task, &shared_value, iterations, false);
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    // 带有同步的加载性能测试
    auto start_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_load_task, &shared_value, iterations, 0);
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end_sync = std::chrono::high_resolution_clock::now();
    auto duration_sync = std::chrono::duration_cast<std::chrono::microseconds>(end_sync - start_sync).count();

    // 不带同步的加载性能测试
    threads.clear();
    auto start_no_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_load_task, &shared_value, iterations, 1);
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end_no_sync = std::chrono::high_resolution_clock::now();
    auto duration_no_sync = std::chrono::duration_cast<std::chrono::microseconds>(end_no_sync - start_no_sync).count();

    threads.clear();
    auto start_nt = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_load_task, &shared_value, iterations, 1);
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end_nt = std::chrono::high_resolution_clock::now();
    auto duration_nt = std::chrono::duration_cast<std::chrono::microseconds>(end_nt - start_nt).count();

    double lat_sync = (double)duration_sync / iterations;
    double lat_no_sync = (double)duration_no_sync / iterations;
    double lat_nt = (double)duration_nt / iterations;
    std::cout << "Load without sync duration: " << lat_no_sync << " microseconds\n";
    std::cout << "Load with sync duration: " << lat_sync << " microseconds"
              << " (" << lat_sync / lat_no_sync << "X)\n";
    std::cout << "Load NT: " << lat_nt << " microseconds"
              << " (" << lat_nt / lat_no_sync << "X)\n";
}

int main() {
    const size_t iterations = 500000; // 设置每个线程的迭代次数
    const size_t thread_count = 32; // 设置线程数量
    benchmark_loads(iterations, thread_count);
    return 0;
}
