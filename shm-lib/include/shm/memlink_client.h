/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2025. All rights reserved.
 * Create: 2025-5-15
 */
#ifndef _MEMLINK_CLIENT_H_
#define _MEMLINK_CLIENT_H_

#include <stdlib.h>
#include <stdbool.h>

struct MetaEntry *BorrowMemMeta(const char *name, int node_ex, int numa_ex,
                                int node_im, int binding_numa, unsigned long size);
void *MemlinkMemMalloc(size_t size_in_mb, int perfLevel, void *attr);
int MemlinkMemFree(void *ptr);
int MemlinkMemShmCreate(const char *name, size_t size_in_mb, int node_id);
void *MemlinkMemShmMmap(void *start, size_t length, int prot, int flags, const char *name, off_t offset, int *result);
int MemlinkMemShmUnmmap(void *start, size_t length, int *result);
int MemlinkMemShmDelete(const char *name);
int MemlinkMemShmRename(const char *oldname, const char *newname);
int MemlinkMemExist(const char *name, bool *exist);
char *MemlinkGetHostInfo(int node_id);
int MemlinkBorrowMem(int node_ex, int numa_ex, int node_im, int binding_numa, size_t size_in_mb);
int MemlinkReturnMem(int node_ex, int numa_ex, int node_im, int binding_numa, size_t size_in_mb);

#endif //_MEMLINK_CLIENT_H_
