#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <getopt.h>
#include <string>
#include <memory>

#include "allocator_interface.h"

constexpr int kThreadCount = 16;
constexpr size_t kDefaultObjectSize = 16;
constexpr size_t kDefaultLoop = 1000000;
constexpr size_t kDefaultTotalSize = 4ULL * 1024 * 1024 * 1024; // 4GB

struct TestConfig {
    size_t object_size = kDefaultObjectSize;
    size_t loop_count = kDefaultLoop;
    double read_ratio = 0.5; // 0~1
    size_t total_size = kDefaultTotalSize;
    const char* cxl_dev_path = "/dev/shm/cxl";
    std::string alloc_type = "lsmalloc";
    int thread_count = kThreadCount;
};

void parse_args(int argc, char** argv, TestConfig& config) {
    int opt;
    while ((opt = getopt(argc, argv, "s:l:r:m:d:a:t:")) != -1) {
        switch (opt) {
            case 's': config.object_size = std::stoul(optarg); break;
            case 'l': config.loop_count = std::stoul(optarg); break;
            case 'r': config.read_ratio = std::stod(optarg); break;
            case 'm': config.total_size = std::stoul(optarg); break;
            case 'd': config.cxl_dev_path = optarg; break;
            case 'a': config.alloc_type = optarg; break;
            case 't': config.thread_count = std::stoi(optarg); break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-s object_size] [-l loop_count] [-r read_ratio] [-m total_size] [-d cxl_dev_path] [-a alloc_type] [-t thread_count] (lsmalloc|cxlshm)\n";
                exit(1);
        }
    }
}

struct Stat {
    std::atomic<size_t> read_count{0};
    std::atomic<size_t> write_count{0};
    std::atomic<size_t> read_latency_ns{0};
    std::atomic<size_t> write_latency_ns{0};
};

// Unified performance test function
void run_performance_test(const TestConfig& config) {
    std::unique_ptr<AllocatorInterface> allocator;
    
    if (config.alloc_type == "lsmalloc") {
        allocator = std::make_unique<LSMallocAllocator>(config.total_size, config.cxl_dev_path);
    } else if (config.alloc_type == "cxlshm") {
        allocator = std::make_unique<CXLShmAllocator>(config.total_size, config.cxl_dev_path);
    } else {
        std::cerr << "Unknown allocator type: " << config.alloc_type << "\n";
        return;
    }

    // Initialize allocator
    allocator->init();
    
    // Allocate memory - each thread allocates objects of different sizes
    std::vector<void*> refs;
    std::vector<size_t> sizes;
    for (int i = 0; i < config.thread_count; ++i) {
        // Each thread can allocate objects of different sizes
        size_t size = config.object_size + (i % 1024); // Example: each thread allocates different size
        void* ref = allocator->allocate(size);
        if (!ref) {
            std::cerr << "Memory allocation failed for thread " << i << "!\n";
            return;
        }
        refs.push_back(ref);
        sizes.push_back(size);
    }

    Stat stat;
    std::atomic<bool> start_flag{false};
    std::vector<std::thread> threads;

    for (int i = 0; i < config.thread_count; ++i) {
        threads.emplace_back([&, i]() {
            void* ref = refs[i]; // Each thread uses its own Ref
            size_t size = sizes[i]; // Each thread uses its own object size
            size_t local_read = 0, local_write = 0;
            size_t local_read_lat = 0, local_write_lat = 0;
            
            while (!start_flag.load()) std::this_thread::yield();
            
            for (size_t j = 0; j < config.loop_count / config.thread_count; ++j) {
                double op = (double)rand() / RAND_MAX;
                if (op < config.read_ratio) {
                    auto t1 = std::chrono::high_resolution_clock::now();
                    allocator->read_operation(ref, size);
                    auto t2 = std::chrono::high_resolution_clock::now();
                    local_read++;
                    local_read_lat += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
                } else {
                    auto t1 = std::chrono::high_resolution_clock::now();
                    allocator->write_operation(ref, size, i);
                    auto t2 = std::chrono::high_resolution_clock::now();
                    local_write++;
                    local_write_lat += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
                }
            }
            stat.read_count += local_read;
            stat.write_count += local_write;
            stat.read_latency_ns += local_read_lat;
            stat.write_latency_ns += local_write_lat;
        });
    }

    auto t_start = std::chrono::high_resolution_clock::now();
    start_flag = true;
    for (auto& t : threads) t.join();
    auto t_end = std::chrono::high_resolution_clock::now();
    
    double total_sec = std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_start).count();
    std::cout << "[" << config.alloc_type << "] Total time: " << total_sec << " s\n";
    if (stat.read_count > 0)
        std::cout << "[" << config.alloc_type << "] Average read latency: " << (stat.read_latency_ns / stat.read_count) << " ns\n";
    if (stat.write_count > 0)
        std::cout << "[" << config.alloc_type << "] Average write latency: " << (stat.write_latency_ns / stat.write_count) << " ns\n";

    // Cleanup
    allocator->cleanup();
}

int main(int argc, char** argv) {
    TestConfig config;
    parse_args(argc, argv, config);
    std::cout << "object_size=" << config.object_size << ", loop_count=" << config.loop_count << ", read_ratio=" << config.read_ratio << ", total_size=" << config.total_size << ", cxl_dev_path=" << config.cxl_dev_path << ", alloc_type=" << config.alloc_type << ", thread_count=" << config.thread_count << "\n";
    
    if (config.thread_count <= 0) {
        std::cerr << "thread_count must be > 0" << std::endl;
        return 1;
    }
    
    run_performance_test(config);
    return 0;
} 