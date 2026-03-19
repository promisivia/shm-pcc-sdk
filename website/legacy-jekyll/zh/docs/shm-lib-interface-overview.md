---
layout: default
title: shm-lib 接口概览（面向 YCSB/BwTree 开发）
nav_order: 6
parent: 用户文档
description: 从 YCSB-C 与 BwTree 的实际接入方式出发，总结 shm-lib 的接口面与典型调用路径（偏“查 API 用”）
lang: zh
permalink: /zh/docs/shm-lib-interface-overview.html
---

{% include language-switcher.html %}

## Overview of shm-lib interfaces

## 0. 最常见的调用路径（YCSB / BwTree 都会用到）

1. （可选）初始化 shm-lib 的全局组件：`initialize_shm_related(machine_no)`（`utils/init.h`）
2. 初始化共享内存 allocator（绑定到全局 `cacheable`）：二选一
   - `init_cacheable_allocator(shm_path, base, size)`（本地文件 / mmap 后端）
   - `init_cxl_cacheable_allocator(shm_path, base, size)`（CXL 场景）
3. 在共享内存上分配与构造对象：
   - 直接：`cacheable.malloc()` + placement new
   - 或者：`ALLOC_AND_CONSTRUCT(type, cacheable.malloc, ...)`
4. 运行期动态数据尽量用 `cxl_vector` / `cxl_string`（其内部扩容也走 `cacheable`）
5. 释放与销毁：
   - placement new 构造：`DESTROY_AND_DEALLOC(obj, Type, cacheable.free)`
   - 若类启用 `NEW_CLASS_ON_SHM` 且用 `new` 构造：直接 `delete`

## 1. Memory Allocation and Management（核心接口）

头文件：`shm-lib/include/shm/mm.h`

### 1.1 allocator 初始化（全局一次）

- `void init_cacheable_allocator(const char* shm_path, void* base, size_t size);`
- `void init_cxl_cacheable_allocator(const char* shm_path, void* base, size_t size);`
- （可选）`void init_uncacheable_allocator();`（仅 `ENABLE_UNCACHE_MEM`）

YCSB 的典型用法：`tests/YCSB-C/ycsbc.cc` 的 `PrepareShmEnv()`。

### 1.2 全局内存池：`cacheable`

`mm.h` 暴露了全局 `MemoryManager cacheable`，在 YCSB/BwTree 里通常直接用它：

- `void* cacheable.malloc(size_t size);`
- `void cacheable.free(void* ptr);`
- `int cacheable.posix_memalign(void** memptr, size_t alignment, size_t size);`
- `int cacheable.clalign(void** memptr, size_t size);`（按 `CACHE_LINE_SIZE` cacheline 对齐）

BwTree 的典型用法：`ds/BwTree/src/bwtree.h` 内部大量调用 `cacheable.malloc/free/clalign`（在 `USE_CXL` 编译路径上尤其明显）。

## 2. Shared-memory STL Types（把容器搬到 shm）

头文件：`shm-lib/include/shm/cxl_type.h`

- `template <typename T> using cxl_vector = std::vector<T, CXLAllocator<T>>;`
- `template <typename T> using cxl_string = std::basic_string<T, ..., CXLAllocator<T>>;`

它们的核心意义是：**容器内部的动态分配（扩容/元素存储）会走 `cacheable`**，避免“对象本体在 shm，但容器扩容跑到堆上”。

YCSB 典型用法：

- `tests/YCSB-C/ycsbc.cc`：`cxl_vector<ycsbc::DB*>`
- `tests/YCSB-C/include/core/delegator/master_trx_manager.h`：操作序列/控制器对象构造

## 3. Object Construction Helpers（placement new 相关宏）

头文件：`shm-lib/include/utils/helper.h`

- `ALLOC_AND_CONSTRUCT(type, allocator, ...)`：分配 + placement new 构造
- `DESTROY_AND_DEALLOC(obj, type, allocator)`：显式析构 + 释放 + 置空
- `NEW_CLASS_ON_SHM`：为类重载 `new/delete`（仅 `USE_CXL` 下生效）

YCSB 典型用法：

- `tests/YCSB-C/ycsbc.cc`：`ALLOC_AND_CONSTRUCT(cxl_vector<DB*>, cacheable.malloc)`、构造 `MasterTransactionManager`
- `tests/YCSB-C/include/core/delegator/master_trx_manager.h`：`DESTROY_AND_DEALLOC(...)`

## 4. Messaging and Queues（常用于线程池/跨机消息）

这一组接口在仓库里主要以两种形态出现：

### 4.1 进程内并发队列（模板类）

头文件：`shm-lib/include/msg/mpmc_queue.h`

- `template <typename T> class MPMCQueue`：`enqueue()` / `dequeue()` / `try_dequeue()` / `empty()` / `size()`

YCSB 典型用法：

- `tests/YCSB-C/include/db/thread_pool.h`：`MPMCQueue<std::function<void()>>` 做任务队列

### 4.2 基于 shm page 的 MsgQueue（固定布局 + 变长消息）

头文件：`shm-lib/include/msg/msg_queue.h`、`shm-lib/include/msg/msg_dispatcher.h`、`shm-lib/include/msg/msg_collector.h`

- `struct msg_node_t` / `msg_type_t`：消息布局与类型
- `class MsgQueue`：`enqueue_msg()` / `dequeue_msg()` 等
- `class MsgDispatcher` / `class MsgCollector`：按 `msg_type_t` 注册 handler 并收发

若你的系统需要跨机/跨进程消息回调，可进一步看：

- `shm-lib/include/shm/mempool.h`：`initialize_shm_and_queue()` / `initialize_comm(machine_no)` / `add_dispatcher(type, handler)`

## 5. Multi-machine Orchestration（YCSB 多机 follower 启动）

头文件：`shm-lib/include/connection/establish.h`

- `class SSHConnection`：封装 libssh session/channel（`ssh_exec()` / `listen()` / `disconnect()`）
- `class FollowerManager`：`start_followers(build_command)` / `stop_followers()`

YCSB 典型用法：

- `tests/YCSB-C/include/core/delegator/master_trx_manager.h`：master 通过 `FollowerManager` 启动 follower

## 6. OCC / Stale / Replica Helpers（按需接入的高级组件）

这些接口在不同系统里是否需要取决于你的并发控制与回收策略：

- `shm-lib/include/occ/occ.h`：`occ::OCC`（`Update()` / `IsMatch()`；按 machine 维护 “ptr→version”）
- `shm-lib/include/clstale/stale.h`：stale ring buffer（`create_stale_list()` / `add_stale()` / `process_stale()` / `recycle_stale()`）
- `shm-lib/include/replica_help_update/help_update.h`：`HelpUpdate<T>`（`cas_ptr()` / `load_ptr()`；用于 replica 指针协助更新）

## 7. 你通常需要 include 哪些头文件？

以“做一个基于 shm 的 YCSB/BwTree 系统”为目标，最常见是这三件套：

- `shm/mm.h`：初始化 allocator + `cacheable`
- `shm/cxl_type.h`：`cxl_vector` / `cxl_string`
- `utils/helper.h`：`ALLOC_AND_CONSTRUCT` / `DESTROY_AND_DEALLOC` / `NEW_CLASS_ON_SHM`

如果你还要做 follower 启动/跨机协同：

- `connection/establish.h`
- `shm/mempool.h` + `msg/*`（queue/dispatcher/collector）

