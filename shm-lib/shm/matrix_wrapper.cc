#include <sys/mman.h>
#include <math.h>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <cstring>
#include <atomic>

#ifdef __cplusplus
extern "C" {
#endif
#include "shm/matrix_wrapper.h"
#include "shm/memlink_client.h"
#ifdef __cplusplus
};
#endif

#define M 1024L*1024
#define ALLOC_SIZE 2L*1024*1024*1024
#define MAX_KIND_LIMIT 5

bool _initialized = false;

int prepare_environments() {
    std::cout << "Use Memlink..." << std::endl;
    return 0;
}


int malloc_remote_memory(const char* name, size_t size) {
    size_t size_in_mb = size / (M);
    int malloc_ret = MemlinkMemShmCreate(name, size_in_mb, -1);
    std::cout << "MemlinkMemShmCreate, name: " << name << " size: " << size << ", error code " << malloc_ret << std::endl;
    return malloc_ret;
}

void* mmap_remote_memory(const char* name, void *base, size_t size) {
    int ret;
    void *addr = MemlinkMemShmMmap(base, size, PROT_READ | PROT_WRITE, MAP_SHARED, name, 0, &ret);
    bool exist;
    int exist_code = MemlinkMemExist(name, &exist);
    std::cout << "MemlinkMemShmMmap, name: " << name << " size " << size << ", addr: " << addr << ", error code " << ret << ", exist " << exist << std::endl;
    return addr;
}

int flush_shared_memory(void* start, size_t size) {
    return 0;
}

int munmap_shared_memory(void* start, size_t size) {
    int error_code;
    int ret = MemlinkMemShmUnmmap(start, size, &error_code);
    std::cout << "MemlinkMemShmUnmmap, start: " << start << " size: " << size << " ret: " << ret << "error code: " << error_code << std::endl;
    return ret;
}

int free_remote_memory(const char* name) {
    int ret = MemlinkMemShmDelete(name);
    std::cout << "MemlinkMemShmDelete, name: " << name << " error code: " << ret << std::endl;

    return ret;
}


void* seek_shared_memory(void* start, size_t off, size_t* size, size_t* offset) {
    return nullptr;
}

int rename_remote_memory(const char* from, const char* to) {
    return 0;
}

int remote_name_exist(const char* name, bool* exist) {
    return 0;
}

int shared_addr_exist(void* addr, bool* exist) {
    *exist = true;
    return 0;
}

int total_memory_info(size_t* used, size_t* alloc, size_t* total) {
    *used = 0;
    *alloc = 0;
    *total = 0;
    return 0;
}
