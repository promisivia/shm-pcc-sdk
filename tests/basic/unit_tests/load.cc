#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <immintrin.h>

#include "../nt.h"

// ���غ���������ͬ��
template <typename T>
T load_with_sync(std::atomic<T>* ptr) {
    clwb(ptr, sizeof(T));
    T value = ptr->load(std::memory_order_seq_cst);
    memory_fence();
    return value;
}

template <typename T>
T nt_load(std::atomic<T>* ptr) {
    T value = READ_NT_64(ptr);
    memory_fence();
    return value;
}

// ���غ���������ͬ��
template <typename T>
T load_without_sync(std::atomic<T>* ptr) {
    T value = ptr->load(std::memory_order_seq_cst);
    memory_fence();
    return value;
}

void benchmark_loads(size_t iterations) {
    std::atomic<int64_t> shared_value(0);

    // Ԥ��
    for (size_t i = 0; i < iterations; ++i) {
        load_without_sync(&shared_value);
    }

    // ����ͬ���ļ������ܲ���
    auto start_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        load_with_sync(&shared_value);
    }
    auto end_sync = std::chrono::high_resolution_clock::now();
    auto duration_sync = std::chrono::duration_cast<std::chrono::microseconds>(end_sync - start_sync).count();

    // ����ͬ���ļ������ܲ���
    auto start_no_sync = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        load_without_sync(&shared_value);
    }
    auto end_no_sync = std::chrono::high_resolution_clock::now();
    auto duration_no_sync = std::chrono::duration_cast<std::chrono::microseconds>(end_no_sync - start_no_sync).count();

        // ����ͬ���ļ������ܲ���
    auto start_nt = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        nt_load(&shared_value);
    }
    auto end_nt = std::chrono::high_resolution_clock::now();
    auto duration_nt = std::chrono::duration_cast<std::chrono::microseconds>(end_nt - start_nt).count();

    std::cout << "Load with sync duration: " << duration_sync << " microseconds\n";
    std::cout << "Load without sync duration: " << duration_no_sync << " microseconds\n";
        std::cout << "Load NT: " << duration_nt << " microseconds\n";
}

int main() {
    const size_t iterations = 1000000; // ���õ�������
    benchmark_loads(iterations);
    return 0;
}
