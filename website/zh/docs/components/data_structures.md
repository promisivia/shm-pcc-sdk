# 数据结构（Data Structures）

SDK 的使用路径通常从两件事开始：

1. 初始化共享内存分配器（`cacheable`），确保对象分配在共享内存上
2. 选择/接入一个数据结构（`ds/**`），并在应用（例如 YCSB-C）里通过统一接口使用

本文以 `tests/YCSB-C/` 作为“可运行的参考实现”，介绍关键接入点。

## 1. 初始化共享内存环境（Allocator）

YCSB-C 在启动时会读 INI 配置并初始化 `cacheable` 分配器（本地文件映射或 CXL 共享内存模式）。核心入口是 `PrepareShmEnv()`：

```{literalinclude} ../../../../tests/YCSB-C/ycsbc.cc
:language: c++
:start-after: void PrepareShmEnv
:end-before: auto build_dbs
```

如果你自己的应用要复用相同能力，通常只需要：

- 包含 `shm/mm.h`
- 在业务逻辑启动前调用 `init_cacheable_allocator()` 或 `init_cxl_cacheable_allocator()`

## 2. 在共享内存上使用 C++ 容器（`cxl_vector` / `cxl_string`）

为了让 `std::vector/std::string` 的内部内存也来自共享内存，仓库提供了基于 `CXLAllocator` 的别名类型：

```{literalinclude} ../../../../shm-lib/include/shm/cxl_type.h
:language: c++
```

当你在共享内存中构建对象时，建议优先使用这些别名类型来承载“会动态增长的内部缓冲区”。

## 3. 在 YCSB-C 里选择数据结构（`-db`）

YCSB-C 通过 `-db <name>` 选择一个 `ycsbc::DB` 适配器；适配器内部再调用对应的数据结构实现（例如 BwTree、Masstree、CLHT 等）。

可选项与映射关系以 `DBFactory::CreateDB()` 为准：

```{literalinclude} ../../../../tests/YCSB-C/db/db_factory.cc
:language: c++
:start-after: DB *DBFactory::CreateDB
:end-before: return NULL;
```

```{note}
`CreateDB()` 中的分支受编译宏控制（例如 `ENABLE_BWTREE_DB`）。YCSB-C 的 `CMakeLists.txt` 会通过 `add_db(...)` 宏把 `ds/**` 的库/头文件引入构建。
```

## 4. 如何接入你自己的数据结构

最小闭环通常是：

1. 在 `tests/YCSB-C/include/db/` 增加一个 `*_db.h`，继承 `ycsbc::DB` 并实现 `ReadInternal/UpdateInternal/InsertInternal/DeleteInternal` 等接口
2. 在 `tests/YCSB-C/db/` 增加对应的 `*_db.cc`，把 YCSB 的操作映射到你的数据结构 API
3. 在 `tests/YCSB-C/db/db_factory.cc` 中注册 `dbname` 分支
4. 在 `tests/YCSB-C/CMakeLists.txt` 中用 `add_db(...)` 把你的数据结构目录（`ds/<YourDS>`）加入构建

如果你的数据结构对象需要放在共享内存上，建议使用 `ALLOC_AND_CONSTRUCT(...)` 这类在共享内存上 placement-new 的方式（参考 `db_factory.cc` 中对 `cacheable.malloc` 的用法）。

## 5. 常见注意事项

- **初始化顺序**：先初始化 `cacheable`，再构造任何需要落在共享内存上的对象/容器。
- **指针与可见性**：共享内存里的指针只在“同一映射布局”下有意义；多机/多进程场景要以仓库提供的启动方式与约束为准。
- **性能定位**：优先用 `tests/YCSB-C/` 的 workload 与脚本复现问题，再回到 `ds/**` 或 `shm-lib/**` 做定点优化。

