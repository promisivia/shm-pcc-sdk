#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cstring>

#include "lsmalloc.h"
#include "cxlmalloc.h"
#include "cxlmalloc-internal.h"

// Abstract base class defining unified interface
class AllocatorInterface {
public:
    virtual ~AllocatorInterface() = default;
    virtual void init() = 0;
    virtual void* allocate(size_t size) = 0;
    virtual void read_operation(void* ref, size_t size) = 0;
    virtual void write_operation(void* ref, size_t size, int thread_id) = 0;
    virtual void cleanup() = 0;
};

// LSMalloc implementation
class LSMallocAllocator : public AllocatorInterface {
private:
    lsmem_t allocator;
    size_t total_size;
    const char* cxl_dev_path;

public:
    LSMallocAllocator(size_t total_sz, const char* dev_path) 
        : total_size(total_sz), cxl_dev_path(dev_path), 
          allocator(nullptr) {}

    void init() override {
        lsmem_attr_t attr;
        lsmem_attr_init(&attr);
        attr.total_size = total_size;
        attr.cxl_dev_path = cxl_dev_path;
        allocator = lsmem_create(&attr);
    }

    void* allocate(size_t size) override {
        SHMRef ref = lsmem_malloc(allocator, size);
        return reinterpret_cast<void*>(static_cast<uintptr_t>(ref));
    }

    void read_operation(void* ref_ptr, size_t size) override {
        SHMRef ref = static_cast<SHMRef>(reinterpret_cast<uintptr_t>(ref_ptr));
        lsmem_read(allocator, ref, 0, size);
    }

    void write_operation(void* ref_ptr, size_t size, int thread_id) override {
        SHMRef ref = static_cast<SHMRef>(reinterpret_cast<uintptr_t>(ref_ptr));
        lsmem_write(allocator, ref, 0, "1", 16);
    }

    void cleanup() override {
        if (allocator) lsmem_destroy(allocator);
    }
};

// CXL SHM implementation
class CXLShmAllocator : public AllocatorInterface {
private:
    void* cxl_mem;
    cxl_shm* shm;
    size_t total_size;
    const char* cxl_dev_path;

public:
    CXLShmAllocator(size_t total_sz, const char* dev_path)
        : total_size(total_sz), cxl_dev_path(dev_path),
          cxl_mem(nullptr), shm(nullptr) {}

    void init() override {
        cxl_mem = get_cxl_mm(cxl_dev_path, total_size);
        shm = new cxl_shm(total_size, cxl_mem);
        shm->thread_init();
    }

    void* allocate(size_t size) override {
        CXLRef* ref = new CXLRef(shm->cxl_malloc(size, 0));
        return reinterpret_cast<void*>(ref);
    }

    void read_operation(void* ref_ptr, size_t size) override {
        CXLRef* ref = reinterpret_cast<CXLRef*>(ref_ptr);
        void* addr = ref->get_addr();
        volatile char tmp = *((volatile char*)addr);
        (void)tmp;
    }

    void write_operation(void* ref_ptr, size_t size, int thread_id) override {
        CXLRef* ref = reinterpret_cast<CXLRef*>(ref_ptr);
        void* addr = ref->get_addr();
        ((volatile char*)addr)[0] = '1';
    }

    void cleanup() override {
        if (shm) delete shm;
    }
}; 
