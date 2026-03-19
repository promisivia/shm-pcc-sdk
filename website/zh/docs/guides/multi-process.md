# 多进程共享同一块地址空间（BwTree/YCSB 经验）

本项目里很多数据结构（尤其是 BwTree）会在共享内存里保存 **原生指针**（例如节点指针、版本指针、根指针）。  
如果想让两个（或多个）进程“直接拿到同一份对象并继续操作”，就必须满足一个关键条件：

- **所有进程把同一份共享内存映射到相同的虚拟地址范围**（同一 `base`），这样进程 A 写入的指针值在进程 B 里仍然指向正确对象。

这页总结项目里已经在用的写法（以 YCSB 的 master/follower 与 BwTree 的 `GetEmptyTree()` 为例），并给出你在自己程序里复用的最小步骤。

## 1. 核心机制：`mmap(base, ..., MAP_SHARED)`

共享内存的“共享”来自两点：

- **同一份 backing store**：例如 DAX 设备（`/dev/dax0.0`）或一个文件（`/dev/shm/cxl`）
- **`MAP_SHARED` 映射**：多个进程映射同一个 fd/offset，看到同一份物理内容

而“指针可跨进程复用”来自：

- **固定虚拟地址**：所有进程都用相同的 `base` 映射同一段共享内存

项目里这个入口在 `PrepareShmEnv()`：从配置读取 `mmap_base_addr`，传给 `init_cacheable_allocator()` / `init_cxl_cacheable_allocator()`。

```{literalinclude} ../../../../tests/YCSB-C/ycsbc.cc
:language: cpp
:start-after: void PrepareShmEnv
:end-before: auto build_dbs
```

配置示例（同一组进程必须一致）：

```{literalinclude} ../../../../tests/YCSB-C/config.ini
:language: ini
:start-after: [shm/cacheable]
:end-before: [shm/uncacheable]
```

## 2. BwTree 是怎么“让两个进程共享一棵树”的？

BwTree 本身并不做“跨进程”魔法，它只假设：

- 树对象和节点对象在同一块可访问的内存里；
- 指针值在当前进程里可解引用。

项目里让它跨进程成立的做法是：**把树对象放进共享内存池**，再把树指针当作“共享句柄”传给其他进程。

`GetEmptyTree()` 的关键点：

- `cacheable.malloc(sizeof(TreeType))`：从共享内存池分配
- placement new：在共享内存上构造 `TreeType`

```{literalinclude} ../../../../ds/BwTree/src/test_suite.cpp
:language: cpp
:start-after: TreeType *GetEmptyTree
:end-before: return t1;
```

只要两个进程的共享内存映射在同一 `base`，那么这个 `TreeType*` 在两个进程里都是同一个地址，树内部的各种指针也都能互相理解。

## 3. YCSB master/follower 是怎么把“共享指针”传给另一个进程的？

YCSB 的 master 会把一个共享结构体指针（`g_var_struct`）以十六进制字符串形式塞进启动命令：

```{literalinclude} ../../../../tests/YCSB-C/include/follower/command.h
:language: cpp
:start-after: ss << "./ycsbc"
:end-before: return ss.str();
```

follower 进程启动后再把该地址字符串 parse 回指针，并直接使用：

```{literalinclude} ../../../../tests/YCSB-C/ycsbc.cc
:language: cpp
:start-after: void follower_process
:end-before: }
```

这套方法成立的前提仍然是：**master 与 follower 的共享内存映射必须落在同一虚拟地址**。

## 4. 两种推荐启动方式

### 4.1 推荐：先映射再 `fork()`（同机）

如果你只是同一台机器上的多进程，最稳妥的方式是：

1. 父进程调用 `PrepareShmEnv()`（完成 `mmap`）
2. 父进程初始化共享对象（例如 `TreeType* tree = GetEmptyTree(true)`）
3. `fork()` 子进程：子进程天然继承同一份映射，地址绝对一致

这样不需要在子进程里再次 `mmap`，也避免了“不同进程 mmap 返回不同地址”的不确定性。

### 4.2 独立启动多个进程：统一 `mmap_base_addr`

如果多个进程是独立启动（例如多个终端/脚本分别启动），你需要做到：

- 所有进程使用同一个 backing store（同一路径/同一个设备）
- 所有进程使用同一个 `mmap_base_addr`
- 尽量保证该地址区间在所有进程里都“空闲可用”（一般选高地址，例如 `0xcaffe0000000`）

## 5. 注意事项（很容易踩坑）

### 5.1 “共享内存”不等于“共享地址”

`MAP_SHARED` 只能保证物理内容共享；如果两个进程映射到不同虚拟地址，内容能共享，但 **指针值不再可用**（因为指针是虚拟地址）。

### 5.2 某些初始化路径会清零映射区（attach 进程要避免）

`SystemMemoryMmapper::allocate()` 在映射成功后会对整段区域 `memset(..., 0, ...)` 以触发预触页/初始化；这意味着：

- 如果你在“第二个 attach 进程”里走这条路径，可能把已有数据清掉。

如果你要做“多进程 attach 到同一份已有数据”，建议确认你使用的初始化路径不会无条件清零，或把“清零/预触页”做成仅 creator 执行的步骤。

### 5.3 把指针当句柄时，必须保证 ABI 一致

参与共享的进程必须使用一致的：

- 代码版本与编译宏（结构体布局、字段条件编译会变）
- 64-bit 进程（指针宽度一致）
- （如果涉及 CXL 切片/多机）一致的 `SimThreadInfo::worker_machine_count` 与 machine id 配置

## 6. 进一步阅读

- [内存分配与对象生命周期](memory-allocation.md)
- [nt 指针与原子语义](nt-pointer.md)
