#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <immintrin.h>

#include "utils/helper.h"

template <typename T>
void store_with_sync(std::atomic<T>* ptr, T value) {
    clwb(ptr, sizeof(T));
    ptr->store(value, std::memory_order_seq_cst);
    memory_fence();
}

template <typename T>
void nt_store(std::atomic<T>* ptr, T value) {
    WRITE_NT_64(ptr, value);
    memory_fence();
}

template <typename T>
void store_without_sync(std::atomic<T>* ptr, T value) {
    ptr->store(value, std::memory_order_seq_cst);
    memory_fence();
}

// 每个线程的加载任务
void thread_store_task(std::atomic<int64_t>* shared_value, int64_t write_value, size_t iterations, int64_t mode) {
    for (size_t i = 0; i < iterations; ++i) {
        if (mode == 0) {
            store_with_sync(shared_value, write_value);
        } else if (mode == 1) {
            store_without_sync(shared_value, write_value);
        } else if (mode == 2) {
            nt_store(shared_value, write_value);
        }
    }
}

void benchmark_stores(size_t iterations, size_t thread_count) {
    std::atomic<int64_t> shared_value(0);
    std::vector<std::thread> threads;
    int64_t write_value = 1;

    // 预热
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_store_task, &shared_value, write_value, iterations, false);
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    // 带有同步的加载性能测试
    auto start_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_store_task, &shared_value, write_value, iterations, 0);
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
        threads.emplace_back(thread_store_task, &shared_value, write_value, iterations, 1);
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end_no_sync = std::chrono::high_resolution_clock::now();
    auto duration_no_sync = std::chrono::duration_cast<std::chrono::microseconds>(end_no_sync - start_no_sync).count();

    threads.clear();
    auto start_nt = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back(thread_store_task, &shared_value, write_value, iterations, 1);
    }
    for (auto& t : threads) {
        t.join();
    }
    auto end_nt = std::chrono::high_resolution_clock::now();
    auto duration_nt = std::chrono::duration_cast<std::chrono::microseconds>(end_nt - start_nt).count();

    double lat_sync = (double)duration_sync / iterations;
    double lat_no_sync = (double)duration_no_sync / iterations;
    double lat_nt = (double)duration_nt / iterations;
    std::cout << "Store without sync duration: " << lat_no_sync << " microseconds\n";
    std::cout << "Store with sync duration: " << lat_sync << " microseconds"
              << " (" << lat_sync / lat_no_sync << "X)\n";
    std::cout << "Store NT: " << lat_nt << " microseconds"
              << " (" << lat_nt / lat_no_sync << "X)\n";
}

int main() {
    const size_t iterations = 100000; // 设置每个线程的迭代次数
    const size_t thread_count = 32; // 设置线程数量
    benchmark_stores(iterations, thread_count);
    return 0;
}
