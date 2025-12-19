#include <stdbool.h>

#ifndef MATRIX_WRAPPER_H
#define MATRIX_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif
#include <cstdlib>

extern int prepare_environments();

extern int malloc_remote_memory(const char* name, size_t size);

extern void* mmap_remote_memory(const char* name, void *base, size_t size);

extern int flush_shared_memory(void* start, size_t size);

extern int munmap_shared_memory(void* start, size_t size);

extern int free_remote_memory(const char* name);

extern void* seek_shared_memory(void* start, size_t off, size_t* size, size_t* offset);

extern int rename_remote_memory(const char* from, const char* to);

extern int remote_name_exist(const char* name, bool* exist);

extern int shared_addr_exist(void* addr, bool* exist);

extern int get_used_size(void* start, size_t* size);

extern int total_memory_info(size_t* used, size_t* alloc, size_t* total);

#ifdef __cplusplus
};
#endif

#endif // MATRIX_WRAPPER_H
