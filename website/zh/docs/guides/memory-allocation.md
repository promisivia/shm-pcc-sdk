# 内存分配与对象生命周期（YCSB / BwTree）

本页介绍如何在SDK中使用CXL内存分配器，来创建对象。
使用方法遵循以下的顺序：**先初始化 allocator，再分配/构造对象；释放时匹配构造方式**。

## 1. 初始化：先配置线程/机器信息，再初始化 allocator（YCSB 参考）

YCSB 的 `PrepareShmEnv` 是最直接参考：从 ini 配置读取共享内存映射参数，并根据 `mem_type` 选择初始化函数。

```{literalinclude} ../../../../tests/YCSB-C/ycsbc.cc
:language: cpp
:start-after: void PrepareShmEnv
:end-before: auto build_dbs
```

关键点：

- **前置条件**：
  - `init_cxl_cacheable_allocator()` 会读取 `SimThreadInfo::worker_machine_count / worker_machine_id` 来决定“每机切片”的视图，调用前需要先完成这些字段的初始化（YCSB 主流程里通常更早就会完成）。
- **函数签名**（在 `shm-lib/include/shm/mm.h`）：
  - `void init_cacheable_allocator(const char *shm_path, void *base, size_t size);`
  - `void init_cxl_cacheable_allocator(const char *shm_path, void *base, size_t size);`
- **共同点**：
  - 都只初始化全局 `cacheable` 分配器，后续所有 `CXLAllocator<T>` / `cxl_vector<T>` / `NEW_CLASS_ON_SHM`（在 `USE_CXL` 打开时）都会走它。
  - `shm_path`：指向底层 mmap 设备或文件（YCSB 里一般是普通文件；CXL 场景典型是 `/dev/dax0.0` 之类的 DAX 设备）。
  - `base`：如果为 `nullptr`，让内核自己选择映射起始地址；非空时配合 `MAP_FIXED` 尝试强制映射到同一虚拟地址（多进程场景才需要考虑）。
  - `size`：整体映射大小；决定 memkind 池能管理的总字节数。
- **`init_cacheable_allocator(...)`**：
  - 使用 `SystemMemoryMmapper`，把整个 `[shm_path, size]` 区间当成一个连续的 mempool。
  - 适合本地/单机场景：不区分 machine id，所有 worker 共享同一块池子。
- **`init_cxl_cacheable_allocator(...)`**：
  - 使用 `CXLSystemMemoryMmapper`，会根据 `SimThreadInfo::worker_machine_count` / `worker_machine_id` 把整个映射按“机器”切片：
    - `per_machine_size = size / worker_machine_count`
    - 当前机器只看到自己的 `[per_machine_base, per_machine_size]` 子区间。
  - 典型用法是 CXL/多机实验：一块物理设备（例如 `/dev/dax0.0`）通过不同 offset 切成 N 份，每机只在自己那一份上分配。
- **和 `USE_CXL` 的关系**：
  - `MemoryManager::malloc/free` 在 `USE_CXL` 打开时会用 `memkind_malloc/memkind_free`，否则 fallback 到普通 `malloc/free`。
  - 也就是说：`init_cxl_cacheable_allocator` 更偏向“如何划分映射区 + per-machine 视图”，而 `USE_CXL` 控制的是“是否真的走 memkind 池”，两个维度可以分别考虑。

### 1.1 ini 配置字段（可直接复用）

下面这份配置被 `tests/YCSB-C/ycsbc.cc` 直接读取，你可以用它作为最小可运行模板：

```{literalinclude} ../../../../tests/YCSB-C/config.ini
:language: ini
:start-after: [shm/cacheable]
```

注意：YCSB 里 `mem_size` 的单位是 **MB**（见 `tests/YCSB-C/ycsbc.cc` 注释与换算逻辑）。

### 1.2 `mmap_base_addr` 与 `MAP_FIXED`（多进程共享必读）

如果你需要在多个进程之间共享“包含裸指针的对象/数据结构”，通常就要求所有进程把共享内存映射到**相同的虚拟地址**（否则指针在不同进程中会失效）。当前实现里，当 `base != nullptr` 时会启用 `MAP_FIXED`：

```{literalinclude} ../../../../shm-lib/shm/mm.cc
:language: cpp
:start-after: // Use MAP_SHARED for multi-process shared memory
:end-before: // Debug: Print actual mmap address
```

这也是为什么配置里会有 `mmap_base_addr`：它是“跨进程地址稳定性”的控制旋钮（详见下文第 4 节）。

## 2. 分配入口：`cacheable` / `CXLAllocator<T>` / `cxl_vector<T>`

头文件：`shm-lib/include/shm/mm.h`

- 全局分配器：`cacheable`
- 常用 API：
  - `cacheable.malloc(size)`
  - `cacheable.free(ptr)`
  - `cacheable.clalign(&ptr, size)`

与 STL 容器配合时，项目提供了基于 `cacheable` 的 allocator/别名：

```{literalinclude} ../../../../shm-lib/include/shm/mm.h
:language: cpp
:start-after: template <typename T> class CXLAllocator
:end-before: void init_cacheable_allocator
```

```{literalinclude} ../../../../shm-lib/include/shm/cxl_type.h
:language: cpp
```

（可选）`uncacheable` 分配器只在编译时打开 `ENABLE_UNCACHE_MEM` 时才存在；如果你确实需要它，建议从 `tests/YCSB-C/ycsbc.cc` 里 `#ifdef ENABLE_UNCACHE_MEM` 的逻辑开始对齐配置与初始化调用。

## 3. 构造与释放的配对规则

### 3.1 ALLOC_AND_CONSTRUCT/DESTROY_AND_DEALLOC

- 构造：`ALLOC_AND_CONSTRUCT(Type, cacheable.malloc)`——底层会先用给定的 `malloc` 分配，再在那块内存上做 placement new。
- 释放：`DESTROY_AND_DEALLOC(ptr, Type, cacheable.free)`——先显式调用 `Type` 析构，再用给定的 `free` 释放那块内存。

宏定义在 `shm-lib/include/utils/helper.h`：

```{literalinclude} ../../../../shm-lib/include/utils/helper.h
:language: cpp
```

这一路径适合共享内存容器和手动生命周期管理对象，尤其是：

- 你希望**显式区分**“对象位于 shm 还是堆上”；
- 你需要把“构造/析构/释放”拆开在不同函数里执行；
- 你在容器/数据结构里存放的是非平凡析构类型（必须保证析构被调用）。

常见错误：

- 用 `delete` 回收 `ALLOC_AND_CONSTRUCT(...)` 得到的对象（错：它不是 `new` 出来的）。
- 只 `cacheable.free(ptr)` 不调用析构（错：会泄露资源或跳过必要清理）。

### 3.2 `NEW_CLASS_ON_SHM`

当启用 `USE_CXL` 时，类内 `NEW_CLASS_ON_SHM` 会重载 `new/delete` 到共享内存路径。  
如果未启用 `USE_CXL`，该宏为空，仍是普通堆分配。

- 宏展开展示（简化理解）：
  - 在 `USE_CXL` 打开时，会往类里注入自定义 `operator new/delete`，内部调用 `cacheable.clalign` / `cacheable.free`；
  - 对这个类而言，**一旦加了 `NEW_CLASS_ON_SHM`，所有 `new`/`delete` 都会走共享内存分配路径**。
- 使用建议：
  - 只给“确实希望长期驻留在共享内存里”的类型加 `NEW_CLASS_ON_SHM`，不要随意乱加；
  - 如果既有堆对象又有 shm 对象，推荐用 `ALLOC_AND_CONSTRUCT` 路径显式区分，减少“这个类到底在什么内存上”的歧义。

## 4. 跨进程共享：虚拟地址稳定性与指针有效性

当你把对象放进共享内存后，最容易踩的坑不是“能不能分配”，而是“**别的进程能不能正确解释这块内存**”。

- **只要对象/数据结构里保存了裸指针**（例如 STL 容器的内部指针、BwTree 里的各种 `Node*`、手写链表指针等），就默认要求：多个进程对同一段共享内存的映射地址一致。
- 如果 `base == nullptr` 让内核自由选择地址，那么每个进程很可能得到不同的虚拟地址；此时“把指针写进共享内存”几乎必然在跨进程读写时失效。

实操建议：

- 多进程共享：在所有进程里配置同一个 `mmap_base_addr`，并确保该地址区间在进程虚拟地址空间里可用；如果 `mmap(..., MAP_FIXED, ...)` 失败或被迫映射到别处，应该把它当成“共享指针语义不成立”的硬错误来处理。
- 单进程使用：即使映射基址不固定也可以工作，但你仍然需要遵守第 1 节的初始化顺序（先 init，再 new/malloc）。

## 5. BwTree：可变长节点与显式销毁

BwTree 内部大量使用“按需分配 + placement new”的模式，尤其是可变长的 elastic node。下面的代码展示了它在 `USE_CXL` 打开时走 `cacheable.malloc`，否则走 `new char[]`，然后用 placement new 初始化对象：

```{literalinclude} ../../../../ds/BwTree/src/bwtree.h
:language: cpp
:start-after: inline static ElasticNode *Get
:end-at: return node_p;
```

对应的释放通常也不是简单 `delete`：

- 先按类型调用析构（如果需要）。
- 再释放底层整块分配（例如通过 `ElasticNode::Destroy()` 间接回收 AllocationMeta 管理的 chunk）。

同一份 BwTree 代码在 `USE_CXL` 开/关时会走不同的分配后端；如果你的目标是“让 BwTree 的核心节点常驻 shm”，就需要确保：

- allocator 已初始化；
- `USE_CXL` 的编译路径确实启用；
- 多进程场景下映射基址稳定（否则节点里保存的指针在别的进程不可用）。

## 6. 常见坑与自检清单

- **全局/静态对象提前构造**：不要在全局构造函数里创建 `cxl_vector`/调用 `cacheable.malloc`；应在 `PrepareShmEnv()`（或等价初始化）之后再创建。
- **“看起来在 shm，实际上在堆”**：`USE_CXL` 未打开时，`cacheable.malloc/free` 会 fallback 到 `::malloc/free`，`NEW_CLASS_ON_SHM` 也为空宏。
- **混用释放方式**：`new/delete`、`malloc/free`、`ALLOC_AND_CONSTRUCT/DESTROY_AND_DEALLOC` 三条路径必须成对；跨路径释放通常都是未定义行为。
- **跨进程指针失效**：只要对象里出现裸指针，就把 `mmap_base_addr` 当成必填项，并确保所有进程一致。
