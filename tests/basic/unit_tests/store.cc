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

void benchmark_stores(size_t iterations) {
    std::atomic<int64_t> shared_value(0);
    int64_t value = 1;

    // 带有同步的加载性能测试
    auto start_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        store_with_sync(&shared_value, value);
    }
    auto end_sync = std::chrono::high_resolution_clock::now();
    auto duration_sync = std::chrono::duration_cast<std::chrono::microseconds>(end_sync - start_sync).count();

    // 不带同步的加载性能测试
    auto start_no_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        store_without_sync(&shared_value, value);
    }
    auto end_no_sync = std::chrono::high_resolution_clock::now();
    auto duration_no_sync = std::chrono::duration_cast<std::chrono::microseconds>(end_no_sync - start_no_sync).count();

        // 不带同步的加载性能测试
    auto start_nt = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        nt_store(&shared_value, value);
    }
    auto end_nt = std::chrono::high_resolution_clock::now();
    auto duration_nt = std::chrono::duration_cast<std::chrono::microseconds>(end_nt - start_nt).count();

    std::cout << "store with sync duration: " << duration_sync << " microseconds\n";
    std::cout << "store without sync duration: " << duration_no_sync << " microseconds\n";
        std::cout << "store NT: " << duration_nt << " microseconds\n";
}

int main() {
    const size_t iterations = 1000000; // 设置迭代次数
    benchmark_stores(iterations);
    return 0;
}
