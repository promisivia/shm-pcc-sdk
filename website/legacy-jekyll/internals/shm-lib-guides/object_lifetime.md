# Object Lifetime（对象生命周期）

`shm-lib` 的核心使用模式是：在 `cacheable` 上分配一块原始内存，然后用 placement new 构造对象。**注意：placement new 构造出来的对象，释放时必须显式析构**。

这页的目标是让你在集成到 YCSB/BwTree 或自研系统时，明确两件事：

- 哪些对象应该用 placement new（以及如何正确销毁）
- 哪些对象可以用 `new/delete`（前提是什么）

## 两种常见模式

### A. placement new（显式析构 + free）

推荐搭配 `ALLOC_AND_CONSTRUCT` / `DESTROY_AND_DEALLOC`：

```{literalinclude} ../../../../shm-lib/include/utils/helper.h
:language: cpp
```

YCSB-C 示例：`MasterTransactionManager` 析构中释放 controller：

```{literalinclude} ../../../../tests/YCSB-C/include/core/delegator/master_trx_manager.h
:language: cpp
:start-after: ~MasterTransactionManager
:end-before: private:
```

### B. 类级别默认 shm new/delete（仅 `USE_CXL`）

当编译启用 `USE_CXL` 时，可以在类中使用 `NEW_CLASS_ON_SHM`，让 `new/delete` 自动走 `cacheable`：

```{literalinclude} ../../../../shm-lib/include/utils/helper.h
:language: cpp
:start-after: "#define NEW_CLASS_ON_SHM"
:end-before: "#else"
```

## 常见坑

- 只 `cacheable.free(ptr)` 而不调用析构：会漏掉对象内部资源释放（例如容器析构等）。
- 容器/对象混用不同 allocator：对象本体在 shm，但其内部 `std::vector` 扩容跑到堆上，最终形成跨域指针与不可控生命周期。

## 经验规则（便于 code review）

- placement new 构造出来的对象：必须能在代码里看到 “析构 + free”（或统一用 `DESTROY_AND_DEALLOC`）。
- 要写进共享内存的数据结构：内部成员尽量避免裸指针；如果必须有指针，明确其指向的内存域与生命周期策略。
