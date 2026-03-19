---
layout: default
title: YCSB-C 如何使用 shm-lib
nav_order: 3
parent: 用户文档
description: 从 tests/YCSB-C 的实际调用出发，梳理 shm-lib 在 YCSB-C 里用到的接口与作用
lang: zh
permalink: /zh/docs/ycsb-shm-lib-interfaces.html
---

{% include language-switcher.html %}

# shm-lib 接口文档

接口主要分为以下几类：
1. 初始化CXL环境
2. 分配内存与构造对象
4. 使用基于 shm 的 STL 容器

## 1. 初始化CXL环境

YCSB-C 主要在以下位置接入 `shm-lib`：

- **共享内存环境初始化**：`tests/YCSB-C/ycsbc.cc` 中的 `PrepareShmEnv()`
- **共享内存分配 + placement new 构造对象**：
  - `tests/YCSB-C/ycsbc.cc`：构造 `cxl_vector<DB*>`、构造 `MasterTransactionManager`
  - `tests/YCSB-C/db/db_factory.cc`：用共享内存构造各类 DB 实例（部分在 `ENABLE_*` 宏下）
  - `tests/YCSB-C/include/core/delegator/master_trx_manager.h`：构造/销毁 `TransactionThreadController`、分配 `GlobalVariables`
- **使用基于 shm 的 STL 容器**：
  - `tests/YCSB-C/include/core/delegator/trx_thread_controller.h`：`cxl_vector<>` 存线程/参数/操作列表等
  - `tests/YCSB-C/include/core/client.h`、`tests/YCSB-C/include/core/insert_client.h` 等：通过 `cxl_vector<DB*>` 传递 DB 集合

## 2. YCSB-C 用到的 shm-lib 接口清单（按模块）

### 2.1 `shm/mm.h`：共享内存管理与分配器

头文件：`shm-lib/include/shm/mm.h`

- **`init_cacheable_allocator(const char* shm_path, void* base, size_t size)`**
  - **用途**：初始化“可缓存(cacheable)”内存池（`cacheable` 全局 `MemoryManager`），典型用于本地（DRAM / mmap 文件）作为后端。
  - **YCSB-C 调用点**：`tests/YCSB-C/ycsbc.cc` 的 `PrepareShmEnv()`（`mem_type == "local"`）。

- **`init_cxl_cacheable_allocator(const char* shm_path, void* base, size_t size)`**
  - **用途**：初始化面向 CXL 场景的 cacheable 内存池（仍然绑定到 `cacheable`），提供 CXL/多机分片等能力（由库内部实现决定）。
  - **YCSB-C 调用点**：`tests/YCSB-C/ycsbc.cc` 的 `PrepareShmEnv()`（`mem_type == "cxl"`）。

- **`init_uncacheable_allocator()`**（仅在 `ENABLE_UNCACHE_MEM` 条件编译下）
  - **用途**：初始化“不可缓存(uncacheable)”内存池（`uncacheable` 全局 `MemoryManager`）。
  - **YCSB-C 调用点**：`tests/YCSB-C/ycsbc.cc` 的 `PrepareShmEnv()`（当配置 `ini["shm"]["enable_uncacheable"]` 为真时）。

- **`MemoryManager` 与全局实例 `cacheable`**
  - **`void* cacheable.malloc(size_t size)`**
    - **用途**：从 cacheable 内存池分配一段原始内存。
    - **YCSB-C 用法**：
      - 作为 `ALLOC_AND_CONSTRUCT(..., cacheable.malloc, ...)` 的 allocator 参数，用于 placement new 构造对象。
      - 直接分配原始结构体空间，例如 `GlobalVariables`。
  - **`void cacheable.free(void* ptr)`**
    - **用途**：释放由 `cacheable` 分配的内存。
    - **YCSB-C 用法**：配合 `DESTROY_AND_DEALLOC(..., cacheable.free)`，以及少量直接 free。
  - **`int cacheable.clalign(void** memptr, size_t size)`**
    - **用途**：按缓存行（`CACHE_LINE_SIZE`）对齐分配，常用于减少 false sharing、提升并发访问效率。
    - **YCSB-C 直接调用**：YCSB-C 自身未直接调用，但会被 `NEW_CLASS_ON_SHM`/容器分配器间接使用。

### 2.2 `shm/cxl_type.h`：基于 `cacheable` 的 STL 容器别名

头文件：`shm-lib/include/shm/cxl_type.h`

- **`template <typename T> using cxl_vector = std::vector<T, CXLAllocator<T>>`**
  - **用途**：把 `std::vector` 的底层分配器替换为 `CXLAllocator<T>`，从而让 vector 的动态扩容、元素存储都走 `cacheable` 内存池。
  - **YCSB-C 用法**：
    - `cxl_vector<ycsbc::DB*>`：保存 DB 实例集合（如 `ycsbc.cc` 的 `build_dbs()`）。
    - `cxl_vector<cxl_vector<Operation>>`：保存每个线程的操作序列（如 `master_trx_manager.h` / `trx_thread_controller.h`）。

- **`template <typename T> using cxl_string = std::basic_string<... CXLAllocator<T>>`**
  - **用途**：与 `cxl_vector` 类似，让 string 的字符缓冲区从 `cacheable` 分配。
  - **YCSB-C 现状**：在当前 `tests/YCSB-C` 路径里未发现直接使用，但属于同一类“把 STL 容器搬到 shm 的模式”。

### 2.3 `utils/helper.h`：在共享内存上构造/销毁对象的宏

头文件：`shm-lib/include/utils/helper.h`

- **`ALLOC_AND_CONSTRUCT(type, allocator, ...)`**
  - **用途**：两步合一：
    - 先用 `allocator(sizeof(type))` 分配原始内存；
    - 再用 `new (addr) type(args...)` 做 placement new 构造对象。
  - **YCSB-C 调用点**：
    - `tests/YCSB-C/ycsbc.cc`：构造 `cxl_vector<ycsbc::DB*>`、构造 `MasterTransactionManager`
    - `tests/YCSB-C/db/db_factory.cc`：构造各类 DB（例如 `BwTreeDB`、`MasstreeDB` 等，取决于编译宏）
    - `tests/YCSB-C/include/core/delegator/master_trx_manager.h`：构造 `TransactionThreadController`
  - **为什么这样做**：这能确保“对象本体 + 对象内部容器扩容内存”都统一来自 `cacheable`，避免跨内存域分配导致的不可控性能/可见性问题。

- **`DESTROY_AND_DEALLOC(obj, type, allocator)`**
  - **用途**：显式调用析构函数 `obj->~type()`，再用 `allocator(obj)` 释放内存，并把指针置空。
  - **YCSB-C 调用点**：`tests/YCSB-C/include/core/delegator/master_trx_manager.h` 的析构函数中销毁 `trx_thread_controller_`。

- **`NEW_CLASS_ON_SHM`（在 `USE_CXL` 下生效）**
  - **用途**：为类重载 `operator new/delete`，让 `new` 自动从 `cacheable` 分配（并可选 cacheline 对齐）。
  - **YCSB-C 现状**：当前 YCSB-C 主要通过 `ALLOC_AND_CONSTRUCT` 来做对象构造；若某些 DB/组件内部采用 `NEW_CLASS_ON_SHM`，则 `new` 也会走 shm。

#### 2.3.1 `ALLOC_AND_CONSTRUCT` vs `NEW_CLASS_ON_SHM` 区别

- **定位不同**
  - **`ALLOC_AND_CONSTRUCT`**：
    - 调用点: 每次构造时
    - 功能: 显式指定 allocator，做“分配 + placement new 构造”，每次构造都可以选择不同 allocator（例如 `cacheable.malloc`）。
    - 对齐粒度：取决于你传入的 allocator（YCSB-C 里常用 `cacheable.malloc`，不等同于 cacheline 对齐）。
    - 释放时需要显式析构再释放内存（YCSB-C 里常配合 `DESTROY_AND_DEALLOC`），否则只 free 会跳过析构逻辑。
  - **`NEW_CLASS_ON_SHM`**：
    - 在类内部重载 `operator new/delete`（仅 `USE_CXL` 下生效），让常规 `new/delete` 自动走 shm 分配与释放。
    - 调用点: 全局一次，一旦对某个类启用，该类后续所有 `new` 默认都走 `cacheable`。
    - 对齐完全取决于你传入的 allocator（YCSB-C 里常用 `cacheable.malloc`，不等同于 cacheline 对齐）。
    - 使用普通 `delete` 即可（先析构，再 `cacheable.free`）。

## 3. 配置项对照：`PrepareShmEnv()` 需要哪些参数

YCSB-C 在 `tests/YCSB-C/ycsbc.cc` 的 `PrepareShmEnv(inicpp::IniManager&)` 读取 ini 中的共享内存配置。关键字段如下（以代码里使用的 key 为准）：

### 3.1 `[shm/cacheable]`

- **`mem_type`**：`local` 或 `cxl`
  - `local` → `init_cacheable_allocator(...)`
  - `cxl` → `init_cxl_cacheable_allocator(...)`
- **`device_path`**：传给初始化函数的 `shm_path`
- **`mmap_base_addr`**：可选；为空则传 `nullptr`，否则解析为地址传入（通常用于固定映射基址）
- **`mem_size`**：单位 **MB**；代码会乘 \(1024 \times 1024\) 转为字节

### 3.2 （可选）uncacheable 配置（`ENABLE_UNCACHE_MEM` 下）

- **`[shm] enable_uncacheable`**：为真则进入 uncacheable 初始化分支
- **`[shm/uncacheable] mem_type`**：当前代码分支只处理 `local`
- 其它字段如 `device_path`、`mmap_base_addr`、`memory_size` 在现有代码中读取，但 `local` 分支实际调用的是 `init_uncacheable_allocator()`（是否使用这些字段由库内部实现决定）

## 4. 典型使用模式（从 YCSB-C 抽象出来）

### 4.1 先初始化 `cacheable`，再构造 shm 上的对象

YCSB-C 的顺序是：

1. 读取 ini/参数
2. 调用 `init_cacheable_allocator(...)` 或 `init_cxl_cacheable_allocator(...)`
3. 通过 `ALLOC_AND_CONSTRUCT(..., cacheable.malloc, ...)` 构造关键对象
4. 用 `cxl_vector<>` 等容器承载运行期动态数据

### 4.2 “对象本体在 shm + 容器元素也在 shm”

如果对象里有 `std::vector`/`std::string` 等动态分配成员，YCSB-C 倾向于使用 `cxl_vector`（以及可选的 `cxl_string`）把这些动态内存也绑定到 `cacheable`，从而：

- **统一内存域**：避免对象在 shm、容器扩容在堆上造成跨域指针/生命周期复杂化
- **对齐与性能**：通过 cacheline 对齐分配器降低 false sharing（具体取决于库实现与使用方式）

