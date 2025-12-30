# PCC Data Structures Demo

This directory contains a minimal demo showing how to use BwTree and ClevelHash as shared memory data structures with a `std::map`-like interface.

## Overview

The demo provides:
- `pcc::btree` - A wrapper around BwTree with `std::map`-like interface
- `pcc::clevelhash` - A wrapper around ClevelHash with `std::unordered_map`-like interface
- Shared memory initialization utilities
- Multi-process support (multiple processes can share the same data structure)

## Building

```bash
cd demos
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Examples

### Simple Demo

Run a simple single-process demo:

```bash
# BwTree demo
./simple_demo

# ClevelHash demo
./simple_demo clevelhash
```

### Multi-Process Demo

Run a demo showing two processes sharing a BwTree:

```bash
# Run both processes (creator and attacher)
./multi_process_demo

# Or run them separately in different terminals:
# Terminal 1:
./multi_process_demo

# Terminal 2:
./multi_process_demo attacher
```

## Usage

### Basic Usage

```cpp
#include "pcc/btree.h"
#include "pcc/init.h"

// Initialize shared memory
pcc::ShmConfig config;
config.shm_path = "/dev/shm/cxl";
config.shm_id = "my_app";
config.shm_size = 1024ULL * 1024 * 1024; // 1GB
config.thread_num = 1;
config.is_creator = true;

pcc::init_shm(config);

// Create or attach to a BwTree
std::string tree_id = pcc::get_shm_id("bwtree", 123);
pcc::btree* tree = pcc::btree::init(tree_id, true, 1);
tree->thread_init(0);

// Use it like std::map
tree->insert(1, 100);
tree->insert(2, 200);

int64_t value;
if (tree->find(1, value)) {
    std::cout << "Found: " << value << std::endl;
}

tree->update(1, 150);
tree->erase(2);

// Cleanup
delete tree;
pcc::cleanup_shm();
```

### Multi-Process Sharing

Process 1 (Creator):
```cpp
pcc::ShmConfig config;
// ... configure ...
config.is_creator = true;
pcc::init_shm(config);

pcc::btree* tree = pcc::btree::init(tree_id, true, 1);
tree->insert(1, 100);
// ... use tree ...
```

Process 2 (Attacher):
```cpp
pcc::ShmConfig config;
// ... configure ...
config.is_creator = false;
pcc::init_shm(config);

pcc::btree* tree = pcc::btree::init(tree_id, false, 1);
int64_t value;
tree->find(1, value); // Can read data inserted by Process 1
// ... use tree ...
```

## API Reference

### pcc::btree

- `static btree* init(const std::string& shm_id, bool is_creator, int thread_num)` - Initialize or attach to shared BwTree
- `static void cleanup(const std::string& shm_id)` - Cleanup shared memory
- `bool insert(const key_type& key, const mapped_type& value)` - Insert or update key-value pair
- `bool update(const key_type& key, const mapped_type& value)` - Update existing key-value pair
- `bool find(const key_type& key, mapped_type& value)` - Find value by key
- `bool erase(const key_type& key)` - Erase key-value pair
- `bool contains(const key_type& key)` - Check if key exists
- `void thread_init(int thread_id)` - Initialize thread for this tree
- `void thread_cleanup(int thread_id)` - Cleanup thread

### pcc::clevelhash

Similar interface to `pcc::btree`, but uses ClevelHash internally.

## Notes

- The shared memory ID (`shm_id`) must be unique for each data structure instance
- Only the creator process should call `cleanup()`
- Each thread that accesses the data structure must call `thread_init()` and `thread_cleanup()`
- The shared memory path (`/dev/shm/cxl`) must exist and be writable

