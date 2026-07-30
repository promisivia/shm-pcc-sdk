---
layout: default
title: User Guide
nav_order: 1
parent: 用户文档
description: Detailed usage instructions for CXL-SDK, including installation, configuration, running examples, and troubleshooting
lang: zh
permalink: /zh/docs/user-guide.html
---


# 用户指南

本文档提供了 CXL-SDK 的详细使用说明，包括安装、配置、运行示例和故障排除。

## Table of Contents

- [Installation Guide](#installation-guide)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Usage Examples](#usage-examples)
- [Performance Tuning](#performance-tuning)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)

## Installation Guide

### System Requirements

#### Minimum Requirements

- **Operating System**: Linux (kernel 4.15+)
- **CPU**: x86_64 or ARM64
- **Memory**: 8GB RAM (16GB+ recommended)
- **Disk Space**: 10GB free space

#### Software Dependencies

**Required:**
- GCC 7+ or Clang 10+ (C++17 support)
- CMake 3.10+
- Make
- Git

**Optional:**
- libnuma-dev (NUMA support)
- libmemkind-dev (CXL memory management)
- libtbb-dev (Intel TBB)
- libssh-dev (SSH support)

### Installation Steps

#### 1. Clone Repository

```bash
git clone https://github.com/promisivia/shm-pcc-sdk.git
cd shm-pcc-sdk
```

#### 2. Install System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libnuma-dev \
    libmemkind-dev \
    libtbb-dev \
    libssh-dev \
    pkg-config
```

**CentOS/RHEL 8+:**
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    numactl-devel \
    tbb-devel \
    libssh-devel \
    git
```

**Arch Linux:**
```bash
sudo pacman -S \
    base-devel \
    cmake \
    numactl \
    tbb \
    libssh
```

#### 3. Build Core Library

```bash
# Build shm-lib
cd shm-lib
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install  # Optional: install to system

cd ../..
```

#### 4. Verify Installation

```bash
# Check library files
ls -la shm-lib/build/libshm-lib.a

# Run simple test
cd tests/basic
./run_tests.sh
```

## Quick Start

### Example 1: Run YCSB-C Benchmark

```bash
cd tests/YCSB-C

# 1. Build project
./build.sh cc

# 2. Run basic test
./run_shm_ds.sh -db=bwtree -mode=test

# 3. View results
cat output.txt
```

### Example 2: Test Different Data Structures

```bash
cd tests/YCSB-C

# Test BwTree
./run_shm_ds.sh -db=bwtree -mode=test

# Test Masstree
./run_shm_ds.sh -db=masstree -mode=test

# Test CLHT
./run_shm_ds.sh -db=clht -mode=test
```

### Example 3: Run STAMP Benchmark

```bash
cd apps/stamp

# Build
./build.sh

# Run Genome application (4 threads)
./run.sh -a genome -t 4

# Run K-Means (8 threads)
./run.sh -a kmeans -t 8
```

## Configuration

### YCSB-C Configuration

#### Build Configuration

`build.sh` supports multiple build modes:

```bash
# Concurrency control mode
./build.sh cc

# No concurrency control mode
./build.sh nocc

# Concurrency control + message queue
./build.sh cc_mq

# Use cxl_std::atomic
./build.sh limit_atomic

# Latency test mode
./build.sh lat_cc      # With concurrency control
./build.sh lat_nocc    # Without concurrency control
```

#### Runtime Configuration

Create `config.ini` file:

```ini
[server]
# Server threads (threads that actually process requests)
threads = 144

# Number of database instances
db_num = 1

# Number of machines (distributed mode)
machine_nr = 1

# Follower list (distributed mode)
follower_list = localhost

[client]
# Client threads (threads that dispatch requests)
threads = 48

[workload]
# Record count
recordcount = 1000000

# Operation count
operationcount = 10000000

# Value size (bytes)
value_size = 8

# Workload file
workload_file = workloads/workloada_zipfian.spec
```

#### Command Line Arguments

`run_shm_ds.sh` supports the following parameters:

```bash
# Specify database type
-db=<type>          # bwtree, masstree, clht, clevelhash, hot, btree_olc, radix_art_olc

# Specify run mode
-mode=<mode>        # test, real, server_thread_scale_test, etc.

# Specify test type
-test_type=<type>   # fixed_db, multi_db, etc.

# Enable message queue
-use-msg=<1|0>      # 1 enable, 0 disable

# Specify config type
-config-type=<type>  # Custom config type
```

### Memory Configuration

#### NUMA Configuration

```bash
# Bind to specific NUMA node
numactl --membind=0 --cpunodebind=0 ./ycsbc -db=bwtree

# Use all NUMA nodes
numactl --interleave=all ./ycsbc -db=bwtree
```

#### CXL Memory Configuration

If using CXL memory:

```bash
# Set CXL memory path
export CXL_MEM_PATH=/dev/cxl/mem0

# Set memory type
export MEMKIND_MEM_TYPE=CXL
```

## Usage Examples

### Example 1: Basic Performance Test

```bash
cd tests/YCSB-C

# 1. Build
./build.sh cc

# 2. Run test (BwTree, 1M records, 10M operations)
./run_shm_ds.sh -db=bwtree -mode=test

# 3. View throughput
grep "Throughput" output.txt

# 4. View latency statistics
grep "Latency" output.txt
```

### Example 2: Thread Scalability Test

```bash
cd tests/YCSB-C

# Build
./build.sh cc

# Run thread scalability test
./run_shm_ds.sh -db=bwtree -mode=server_thread_scale_test

# Results will show performance at different thread counts
```

### Example 3: Real Workload Test

```bash
cd tests/YCSB-C

# Build
./build.sh cc

# Run real workload (using predefined workload files)
./run_shm_ds.sh -db=bwtree -mode=real-100m

# View results
cat log/real/cc.log
```

### Example 4: Latency Analysis

```bash
cd tests/YCSB-C

# Build latency test version
./build.sh lat_cc

# Run latency test
./run_shm_ds.sh -mode=latency_overhead -db=bwtree

# View latency distribution
cat log/latency_overhead/lat_cc_bwtree.log
```

### Example 5: Using Atomic Operations Library

```cpp
#include <cxl_std/atomic.hpp>
#include <iostream>
#include <thread>
#include <vector>

using namespace cxl_std;

int main() {
    atomic<int> counter(0);
    const int num_threads = 4;
    const int increments_per_thread = 1000000;
    
    std::vector<std::thread> threads;
    
    // Launch multiple threads for increment operations
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, increments_per_thread]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.fetch_add(1);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Final counter value: " << counter.load() << std::endl;
    std::cout << "Expected: " << num_threads * increments_per_thread << std::endl;
    
    return 0;
}
```

Compile and run:

```bash
cd atomic
mkdir build && cd build
cmake ..
make
./bin/example
```

## Performance Tuning

### 1. Thread Configuration

**Server Threads:**
- Recommended: 1-2x CPU cores
- For I/O-intensive workloads, can be set higher

**Client Threads:**
- Usually 1/3 to 1/2 of server threads
- Adjust based on workload characteristics

### 2. NUMA Optimization

```bash
# Check NUMA topology
numactl --hardware

# Bind to specific NUMA node (reduce cross-node access)
numactl --membind=0 --cpunodebind=0 ./ycsbc -db=bwtree

# Interleave across all NUMA nodes (increase bandwidth)
numactl --interleave=all ./ycsbc -db=bwtree
```

### 3. CPU Affinity

```bash
# Bind to specific CPU cores
taskset -c 0-15 ./ycsbc -db=bwtree

# Use all cores but set affinity
taskset -c 0-$(($(nproc)-1)) ./ycsbc -db=bwtree
```

### 4. Compiler Optimization

Enable optimizations in `CMakeLists.txt`:

```cmake
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native")
```

### 5. Memory Pre-allocation

For large workloads, pre-allocate memory:

```bash
# Set pre-allocation size (MB)
export PREALLOC_SIZE=4096

# Run test
./run_shm_ds.sh -db=bwtree -mode=test
```

## Troubleshooting

### Issue 1: Compilation Errors

**Symptom:** `error: 'atomic' is not a member of 'std'`

**Solution:**
```bash
# Ensure C++17 standard is used
export CXXFLAGS="-std=c++17"
cmake .. -DCMAKE_CXX_STANDARD=17
```

### Issue 2: Linking Errors

**Symptom:** `undefined reference to 'numa_*'`

**Solution:**
```bash
# Install NUMA development library
sudo apt-get install libnuma-dev  # Ubuntu/Debian
sudo yum install numactl-devel   # CentOS/RHEL

# Rebuild
cd build
cmake ..
make clean
make
```

### Issue 3: Runtime Errors

**Symptom:** `Failed to allocate memory`

**Solution:**
```bash
# Check available memory
free -h

# Reduce workload size
# Edit config.ini, reduce recordcount

# Or use swap space
sudo swapon --show
```

### Issue 4: Poor Performance

**Checklist:**
1. Confirm Release build mode is used
2. Check CPU frequency is normal (`cpupower frequency-info`)
3. Ensure no other processes are consuming resources (`htop`)
4. Check NUMA configuration is correct
5. Confirm compiler optimization flags are enabled

### Issue 5: Test Failures

**Debug Steps:**
```bash
# 1. Build with Debug mode
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 2. Enable verbose output
export DBG_LEVEL=2

# 3. Run test
./run_shm_ds.sh -db=bwtree -mode=test -debug=2

# 4. View logs
cat log/*.log
```

## FAQ

### Q1: What operating systems are supported?

A: Currently primarily supports Linux systems (Ubuntu 20.04+, CentOS 8+, RHEL 8+). Windows and macOS support is under development.

### Q2: Do I need CXL hardware?

A: No. The project can run on standard x86_64 systems. CXL-specific features are optional.

### Q3: How to choose the right data structure?

A: 
- **BwTree**: Suitable for read-heavy workloads
- **Masstree**: Suitable for mixed workloads
- **CLHT**: Suitable for high-concurrency hash table scenarios
- **ClevelHash**: Suitable for scenarios requiring persistence
- **HOT**: Suitable for string key scenarios

### Q4: How to contribute code?

A: Please see [Developer Guide](developer-guide.md) and [Contributing Guide](contributing.md).

### Q5: Where are the performance benchmarks?

A: Performance benchmark results can be found in `tests/YCSB-C/log/` directory. You can also run `./build_run.sh` to generate a complete performance report.

### Q6: What workloads are supported?

A: Supports YCSB standard workloads (A-F) as well as custom workloads. Workload files are located in `tests/YCSB-C/workloads/`.

---

**Need more help?** Please see [API Reference](api/index.md) or open an [Issue](https://github.com/promisivia/shm-pcc-sdk/issues).
