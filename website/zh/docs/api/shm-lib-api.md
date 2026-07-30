---
layout: default
title: shm-lib func API 参考
nav_order: 5
parent: 用户文档
description: 按头文件分组列出 shm-lib 暴露的函数接口（func API），并补充关键参数含义与使用约束
lang: zh
permalink: /zh/docs/shm-lib-api.html
---

{% include language-switcher.html %}

## 说明与范围

本页模仿 NCCL 的 API Reference 风格（例如 `Communicator Creation and Management Functions`），把 `shm-lib/include/` 中对外暴露的 **函数（func）接口** 按头文件分组列出，便于“像查 NCCL 一样查 shm-lib”。

约定：

- **func API**：以“可调用的函数”为主（包括 `extern` 函数、自由函数、以及少量以“类方法形式暴露但实际作为模块 API 使用”的入口）。
- **宏与类型别名**：不作为 func API 的主体，但在与生命周期/分配强相关时会在对应模块下给出“相关宏/类型”提示。
- **签名来源**：均来自仓库内 `shm-lib/include/**` 头文件（以当前仓库版本为准）。

如果你更想看“如何从实际代码里使用这些 API”，请看：

- `shm-lib` 最小程序教程：{doc}`shm-lib-step-by-step`
- 面向 YCSB / BwTree 开发者的接口速览（含 `nt<T>` / `nt_pointer<T>`、message queue）：{doc}`shm-lib-interfaces-ycsb-bwtree`

## 1. `shm/mm.h`：allocator 初始化与 `cacheable`

头文件：`shm-lib/include/shm/mm.h`

### 1.1 `init_cacheable_allocator`

```cpp
void init_cacheable_allocator(const char *shm_path, void *base, size_t size);
```

- **`shm_path`**：共享内存后端路径（常见是 mmap 文件路径或设备路径；具体语义由 `SystemMemoryMmapper` 实现决定）。
- **`base`**：期望映射基址；传 `nullptr` 表示让系统选择。
- **`size`**：内存池大小（字节）。

### 1.2 `init_cxl_cacheable_allocator`

```cpp
void init_cxl_cacheable_allocator(const char *shm_path, void *base, size_t size);
```

- **`shm_path`**：CXL 相关的设备/路径。
- **`base`**：期望映射基址；`nullptr` 表示让系统选择。
- **`size`**：内存池大小（字节）。

### 1.3 `init_uncacheable_allocator`

```cpp
void init_uncacheable_allocator();
```

- **用途**：初始化 `uncacheable` 内存池（仅当编译启用 `ENABLE_UNCACHE_MEM` 时有效）。

### 1.4 相关对象：全局 `cacheable`（以及可选 `uncacheable`）

`mm.h` 中声明了全局内存管理器：

```cpp
extern MemoryManager cacheable;
#ifdef ENABLE_UNCACHE_MEM
extern MemoryManager uncacheable;
#endif
```

`MemoryManager` 的成员方法是你在程序中最常用的分配/释放入口：

- **`void* cacheable.malloc(size_t size)`**
- **`void cacheable.free(void* ptr)`**
- **`int cacheable.posix_memalign(void** memptr, size_t alignment, size_t size)`**
- **`int cacheable.clalign(void** memptr, size_t size)`**：按 `CACHE_LINE_SIZE` 进行 cacheline 对齐分配（对降低 false sharing 常有帮助）。

> 说明：这几个方法属于类方法，不是自由函数；但它们构成了 `shm-lib` 的核心分配 API，所以这里以“API Reference”的方式一并列出。

## 2. `utils/init.h`：初始化入口（与 shm/queue/comm 相关）

头文件：`shm-lib/include/utils/init.h`

### 2.1 `initialize_shm_related`

```cpp
void initialize_shm_related(int machine_no);
```

- **`machine_no`**：机器编号（多机/多进程场景里由上层系统定义编号策略）。
- **用途**：初始化 `shm-lib` 相关的全局状态（通常与 shm、队列、跨机通信初始化有关）。

## 3. `shm/mempool.h`：共享内存 + 队列 + 通信初始化（模块级入口）

头文件：`shm-lib/include/shm/mempool.h`

### 3.1 `initialize_shm_and_queue`

```cpp
void initialize_shm_and_queue();
```

- **用途**：初始化共享内存与消息队列（具体细节取决于实现与编译宏）。

### 3.2 `initialize_comm`

```cpp
void initialize_comm(int machine_no);
```

- **`machine_no`**：机器编号。
- **用途**：初始化跨机通信相关组件。

### 3.3 `add_dispatcher`

```cpp
void add_dispatcher(msg_type_t type, MsgHandler handler);
```

- **`type`**：消息类型（见 `msg/msg_queue.h` 的 `msg_type_t`）。
- **`handler`**：回调处理函数（见 `msg/msg_dispatcher.h` 的 `MsgHandler`）。

### 3.4 `marker_free_cross_machine`

```cpp
void marker_free_cross_machine(memkind_t kind, void* ptr);
```

- **`kind`**：memkind 内存域/池（来自 `memkind.h`）。
- **`ptr`**：待释放指针。
- **用途**：跨机释放/标记释放（用于多机内存回收场景）。

### 3.5 `get_ptr_machine_index`

```cpp
int get_ptr_machine_index(void* ptr);
```

- **`ptr`**：任意指针。
- **返回**：指针所属机器索引（用于跨机寻址/回收逻辑）。

## 4. `msg/msg_queue.h`：消息队列（类方法为主）

头文件：`shm-lib/include/msg/msg_queue.h`

`MsgQueue` 对外可用的主要方法（类 API）：

- **`MsgQueue(void* q_addr)`**
- **`void init_msg_queue()`**
- **`int get_remain_space() const`**
- **`bool is_queue_empty() const`**
- **`int enqueue_msg(msg_node_t* node_to_enqueue)`**
- **`msg_node_t* dequeue_msg(int blocking)`**

相关类型：

- **`msg_type_t`**：`FREE` / `LOCK_DELE` / `MSG_TYPE_NUM`
- **`msg_node_t`**：变长消息节点（含 `content[]`）

## 5. `clstale/stale.h`：stale 列表与回收（自由函数入口）

头文件：`shm-lib/include/clstale/stale.h`

### 5.1 `create_stale_list`

```cpp
void create_stale_list(void* buffer, size_t size);
```

- **`buffer`**：stale ring buffer 的起始地址。
- **`size`**：buffer 大小（字节；要求/约束由实现决定）。

### 5.2 `apply_stale`

```cpp
void apply_stale(void* addr);
```

- **`addr`**：待应用 stale 的地址。

### 5.3 `add_stale`

```cpp
void add_stale(void* addr);
```

- **`addr`**：将地址加入 stale 列表。

### 5.4 `process_stale`

```cpp
void* process_stale(void* arg);
```

- **用途**：后台处理 stale（通常作为线程入口函数）。

### 5.5 `recycle_stale`

```cpp
void* recycle_stale(void* arg);
```

- **用途**：后台回收 stale（通常作为线程入口函数）。

## 6. `utils/cpu_dist.h`：CPU/NUMA 相关辅助函数

头文件：`shm-lib/include/utils/cpu_dist.h`

### 6.1 `get_cpu_nr_numa`

```cpp
int get_cpu_nr_numa(int node);
```

### 6.2 `get_first_cpu_of_numa_node`

```cpp
int get_first_cpu_of_numa_node(int node);
```

### 6.3 `get_available_cpu_server`

```cpp
int get_available_cpu_server();
```

### 6.4 `set_pthread_affinity_attr`

```cpp
int set_pthread_affinity_attr(int cpu, pthread_attr_t *attr);
```

### 6.5 `set_pthread_affinity`

```cpp
int set_pthread_affinity(int cpu);
```

## 7. `utils/rlock.h`：rlock（自由函数入口）

头文件：`shm-lib/include/utils/rlock.h`

### 7.1 `rlock_lock`

```cpp
void rlock_lock(rlock_t* l, lock_t lock, owner_t owner);
```

### 7.2 `rlock_unlock`

```cpp
void rlock_unlock(rlock_t* l);
```

### 7.3 `rlock_st`

```cpp
void rlock_st(rlock_t* l, const lock_t value);
```

### 7.4 `rlock_ld`

```cpp
lock_t rlock_ld(rlock_t* l);
```

### 7.5 `rlock_cas`

```cpp
lock_t rlock_cas(rlock_t* l, lock_t expected, lock_t desired);
```

### 7.6 `rlock_tas`

```cpp
lock_t rlock_tas(rlock_t* l);
```

## 8. `utils/timing.h`：统计初始化与输出

头文件：`shm-lib/include/utils/timing.h`

### 8.1 `InitStatistics`

```cpp
extern void InitStatistics();
```

### 8.2 `PrintStatistics`

```cpp
extern void PrintStatistics();
```

## 9. 与“创建程序”强相关但不属于 func API 的内容

下面这些不是函数（或不以函数形式暴露），但在实际写程序时经常会一起用到：

### 9.1 `utils/helper.h`：对象生命周期宏

头文件：`shm-lib/include/utils/helper.h`

- **`ALLOC_AND_CONSTRUCT(type, allocator, ...)`**：分配 + placement new 构造
- **`DESTROY_AND_DEALLOC(obj, type, allocator)`**：显式析构 + 释放 + 指针置空
- **`NEW_CLASS_ON_SHM`**：为类注入 `operator new/delete`（仅 `USE_CXL` 下生效）

### 9.2 `shm/cxl_type.h`：STL 容器别名

头文件：`shm-lib/include/shm/cxl_type.h`

- **`cxl_vector<T>`**：`std::vector<T, CXLAllocator<T>>`
- **`cxl_string<T>`**：`std::basic_string<T, ..., CXLAllocator<T>>`
