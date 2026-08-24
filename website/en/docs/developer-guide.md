# Developer Guide


This guide is for developers who want to contribute to SHM-PCC-SDK, understand the codebase structure, and extend the project.

## Table of Contents

- [Codebase Structure](#codebase-structure)
- [Build System](#build-system)
- [Code Style](#code-style)
- [Architecture Overview](#architecture-overview)
- [Adding New Features](#adding-new-features)
- [Testing](#testing)
- [Debugging](#debugging)
- [Performance Considerations](#performance-considerations)

## Codebase Structure

### Directory Layout

```
shm-pcc-sdk/
├── atomic/              # Atomic operations library
│   ├── include/        # Public headers
│   ├── src/            # Implementation
│   └── tests/          # Unit tests
├── shm-lib/            # Core shared memory library
│   ├── include/        # Public headers
│   ├── shm/           # Shared memory management
│   ├── msg/           # Message queues
│   ├── utils/         # Utility functions
│   └── connection/    # Connection management
├── ds/                 # Data structures
│   ├── BwTree/        # BwTree implementation
│   ├── Masstree/      # Masstree implementation
│   └── ...            # Other data structures
├── malloc/            # Memory allocators
├── stm/               # Software transactional memory
├── tests/             # Test suite
└── docs/              # Documentation
```

### Key Components

#### 1. Atomic Operations Library (`atomic/`)

Provides multiple atomic operation implementations:
- Standard `std::atomic` wrapper
- Indirect table-based atomic
- Hash table-based atomic

**Key Files:**
- `include/cxl_std/atomic.hpp` - Main atomic interface
- `src/atomic_impl_*.cpp` - Implementation files

#### 2. Shared Memory Library (`shm-lib/`)

Core library for shared memory management:
- Memory allocation and management
- Message queues (MPMC, SPSC)
- Connection establishment
- Utility functions

**Key Files:**
- `include/shm/mempool.h` - Memory pool interface
- `include/msg/mpmc_queue.h` - MPMC queue
- `shm/mempool.cc` - Memory pool implementation

#### 3. Data Structures (`ds/`)

Various concurrent data structure implementations:
- Each data structure is self-contained
- Follows common interface pattern
- Can be used independently

#### 4. Test Suite (`tests/`)

- `YCSB-C/` - YCSB benchmark implementation
- `basic/` - Basic functionality tests
- `correctness_test/` - Correctness verification
- `real-workloads/` - Real-world workload tests

## Build System

### CMake Structure

The project uses CMake for building. Key CMake files:

- `shm-lib/CMakeLists.txt` - Core library build
- `tests/YCSB-C/CMakeLists.txt` - YCSB-C benchmark build
- `atomic/CMakeLists.txt` - Atomic library build

### Building Components

#### Build Core Library

```bash
cd shm-lib
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

#### Build Tests

```bash
cd tests/YCSB-C
./build.sh cc  # or nocc, cc_mq, etc.
```

### CMake Variables

- `CMAKE_BUILD_TYPE`: `Debug` or `Release`
- `CMAKE_CXX_STANDARD`: C++ standard (17)
- `USE_CXL`: Enable CXL-specific features
- `NO_CC`: Disable concurrency control

### Adding New Components

To add a new component:

1. Create directory structure
2. Add `CMakeLists.txt`
3. Define source files
4. Link dependencies
5. Add to parent `CMakeLists.txt`

Example:

```cmake
# CMakeLists.txt for new component

cmake_minimum_required(VERSION 3.10)
project(MyComponent)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_library(mycomponent STATIC ${SOURCES} ${HEADERS})

target_include_directories(mycomponent PUBLIC include)
target_link_libraries(mycomponent PUBLIC shm-lib)
```

## Code Style

### C++ Style Guidelines

1. **Naming Conventions**
   - Classes: `PascalCase` (e.g., `MemoryPool`)
   - Functions: `snake_case` (e.g., `allocate_memory`)
   - Variables: `snake_case` (e.g., `thread_count`)
   - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_THREADS`)
   - Private members: `snake_case_` with trailing underscore

2. **File Organization**
   - One class per file (when possible)
   - Header files: `.h` or `.hpp`
   - Source files: `.cpp` or `.cc`
   - Headers should be self-contained

3. **Code Formatting**
   - Use 4 spaces for indentation
   - Maximum line length: 100 characters
   - Use braces for all control structures
   - Consistent spacing around operators

4. **Comments**
   - Use `//` for single-line comments
   - Use `/* */` for multi-line comments
   - Document public APIs with Doxygen-style comments
   - Explain "why" not "what"

### Example Code Style

```cpp
// Good example
class MemoryPool {
public:
    /**
     * Allocate memory block of specified size.
     * 
     * @param size Size in bytes to allocate
     * @return Pointer to allocated memory, or nullptr on failure
     */
    void* allocate(size_t size) {
        if (size == 0) {
            return nullptr;
        }
        
        // Implementation here
        return allocate_internal(size);
    }

private:
    void* allocate_internal(size_t size);
    size_t pool_size_;
    void* pool_base_;
};
```

## Architecture Overview

### Shared Memory Model

The project uses a shared memory model where:
- Multiple processes/threads share memory
- Lock-free or lock-based synchronization
- NUMA-aware memory allocation
- CXL memory support (optional)

### Concurrency Control

Two main modes:
1. **With Concurrency Control (CC)**: Uses OCC or other CC mechanisms
2. **Without Concurrency Control (NOCC)**: Direct access, faster but less safe

### Data Structure Interface

Common interface pattern for data structures:

```cpp
template<typename Key, typename Value>
class DataStructure {
public:
    // Insert/put operation
    bool put(const Key& key, const Value& value);
    
    // Get operation
    bool get(const Key& key, Value& value);
    
    // Delete operation
    bool remove(const Key& key);
    
    // Scan operation (if supported)
    size_t scan(const Key& start, size_t count, 
                std::vector<std::pair<Key, Value>>& results);
};
```

## Adding New Features

### Adding a New Data Structure

1. **Create Directory Structure**
   ```bash
   mkdir -p ds/MyDS/include
   mkdir -p ds/MyDS/src
   ```

2. **Implement Interface**
   - Create header file with public API
   - Implement core functionality
   - Add error handling

3. **Add to Build System**
   - Create `CMakeLists.txt`
   - Add to YCSB-C build if needed

4. **Add Tests**
   - Create test file
   - Add to test suite
   - Verify correctness

5. **Add Documentation**
   - Document API
   - Add usage examples
   - Update main README

### Adding a New Benchmark

1. **Create Benchmark Directory**
   ```bash
   mkdir -p tests/my_benchmark
   ```

2. **Implement Benchmark**
   - Create main file
   - Implement workload
   - Add result collection

3. **Add Build Configuration**
   - Add to CMakeLists.txt
   - Add build script if needed

4. **Add Documentation**
   - Document how to run
   - Document expected results

## Testing

### Unit Tests

Create unit tests for individual components:

```cpp
#include <gtest/gtest.h>
#include "my_component.h"

TEST(MyComponentTest, BasicFunctionality) {
    MyComponent component;
    EXPECT_TRUE(component.initialize());
    EXPECT_EQ(component.get_count(), 0);
}
```

### Integration Tests

Test component interactions:

```cpp
TEST(IntegrationTest, DataStructureWithMemoryPool) {
    MemoryPool pool(1024 * 1024);
    MyDataStructure ds(&pool);
    
    // Test operations
    ds.put(1, "value1");
    std::string value;
    EXPECT_TRUE(ds.get(1, value));
    EXPECT_EQ(value, "value1");
}
```

### Running Tests

```bash
# Run all tests

cd tests
./run_all_tests.sh

# Run specific test

cd tests/basic
./run_tests.sh
```

### Test Best Practices

1. **Test Coverage**
   - Test normal cases
   - Test edge cases
   - Test error cases
   - Test concurrent access

2. **Test Organization**
   - One test per feature
   - Clear test names
   - Independent tests
   - Fast execution

3. **Test Data**
   - Use fixtures for common setup
   - Use test data generators
   - Clean up after tests

## Debugging

### Debug Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

### Debugging Tools

1. **GDB**
   ```bash
   gdb ./ycsbc
   (gdb) break main
   (gdb) run -db=bwtree
   ```

2. **Valgrind** (Memory Leaks)
   ```bash
   valgrind --leak-check=full ./ycsbc -db=bwtree
   ```

3. **Thread Sanitizer**
   ```bash
   cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread"
   make
   ```

4. **Address Sanitizer**
   ```bash
   cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address"
   make
   ```

### Logging

Use logging for debugging:

```cpp
#ifdef DEBUG
#define DBG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DBG_LOG(fmt, ...)
#endif
```

## Performance Considerations

### Memory Management

1. **Avoid Unnecessary Allocations**
   - Reuse memory pools
   - Pre-allocate when possible
   - Use stack allocation for small objects

2. **Cache Awareness**
   - Consider cache line size (64 bytes)
   - Avoid false sharing
   - Use padding for alignment

3. **NUMA Awareness**
   - Allocate memory on local NUMA node
   - Bind threads to CPUs
   - Use NUMA-aware allocators

### Concurrency

1. **Lock-Free Algorithms**
   - Use atomic operations
   - Minimize contention
   - Consider lock-free data structures

2. **Thread Safety**
   - Document thread safety guarantees
   - Use appropriate synchronization
   - Test concurrent access

3. **Scalability**
   - Design for multi-core
   - Minimize shared state
   - Use per-thread data structures

### Optimization Tips

1. **Profile First**
   - Use `perf` or `gprof`
   - Identify bottlenecks
   - Optimize hot paths

2. **Compiler Optimizations**
   - Use `-O3` for release builds
   - Use `-march=native` for target CPU
   - Enable link-time optimization (`-flto`)

3. **Avoid Premature Optimization**
   - Write clear code first
   - Measure before optimizing
   - Optimize based on data

## Code Review Process

### Before Submitting

1. **Self-Review**
   - Review your own code
   - Run tests
   - Check style
   - Update documentation

2. **Prepare PR**
   - Write clear description
   - Reference issues
   - Add tests
   - Update documentation

### Review Checklist

- [ ] Code follows style guidelines
- [ ] Tests are added/updated
- [ ] Documentation is updated
- [ ] No hardcoded values
- [ ] Error handling is proper
- [ ] Memory management is correct
- [ ] Thread safety is considered
- [ ] Performance is acceptable
- [ ] Code compiles without warnings

## Common Patterns

### Memory Pool Pattern

```cpp
class MemoryPool {
public:
    void* allocate(size_t size) {
        // Allocate from pool
    }
    
    void deallocate(void* ptr) {
        // Return to pool
    }
};
```

### Lock-Free Pattern

```cpp
class LockFreeQueue {
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    
public:
    void enqueue(Item item) {
        // Lock-free enqueue
    }
};
```

### RAII Pattern

```cpp
class LockGuard {
    Lock& lock_;
public:
    LockGuard(Lock& lock) : lock_(lock) {
        lock_.acquire();
    }
    
    ~LockGuard() {
        lock_.release();
    }
};
```

lang: en
---

For more information, see [API Reference](API_REFERENCE.md) or open an [Issue](https://github.com/your-org/shm-pcc-sdk/issues).

