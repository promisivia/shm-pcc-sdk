## RAMCloud 本身的核心特性

RAMCloud 原生设计的重点不是 CXL，而是“分布式 DRAM 上的低延迟、强一致对象存储”：

- 全内存数据面：主数据常驻 DRAM，追求微秒级远程访问延迟。
- 分布式扩展：通过很多台服务器聚合内存容量，对外提供统一的大规模对象存储。
- 持久化与快速恢复：主副本在内存中服务，请求落日志并复制到 backup，节点故障后依靠日志重放和并行恢复快速拉起。
- 强一致对象模型：基本模型是按表组织的 key-value/object store，写入成功后立即可见并保持一致。
- 多表与分片管理：Coordinator、TabletManager、ObjectFinder 等组件负责表、tablet、主节点映射和迁移。
- 日志结构存储：对象写入 `ObjectManager` 后追加到 log，再更新哈希索引；cleaner 负责回收和压缩段。
- 事务与幂等 RPC：支持跨对象事务、去重的 RPC 结果记录、线性一致语义。
- 二级索引与枚举：支持 secondary index、对象遍历、批量/多对象操作。
- 高性能网络路径：RPC、transport、zero-copy region、seglet/segment 注册等机制共同服务低延迟访问。

## 代码里的主要服务结构

当前仓库里，Master 路径仍然是典型 RAMCloud 组织：

```text
Master
  |- ObjectManager
  |- TabletManager
  |- TransactionManager
  |- TxRecoveryManager
  |- UnackedRpcResults
  |- MasterTableMetadata
  |- ServerId / ServerConfig
```

## CXL 新加入的 feature

结合当前代码，这个 fork 已经加入的 CXL 相关能力主要有下面几类。

### 1. CXL 固定内存池管理

- 新增 `MemoryManager` 单例，负责管理 CXL 内存池。
- 通过 `open + mmap(MAP_SHARED)` 映射 CXL 设备文件，再用 `memkind_create_fixed` 把这块地址空间变成固定内存池。
- 新增 `cxl_malloc`、`cxl_memalign`、`cxl_strdup`、`cxl_free`，为后续把对象、日志或元数据放到 CXL 上提供统一分配接口。
- 当 `gUseCXL` 为 false 或内存池未初始化时，会自动退回普通 `malloc/posix_memalign/free`。

### 2. CXL 设备配置参数

- `ServerConfig` 新增了 `cxlMemoryPath` 和 `cxlMemorySize`。
- 这说明系统层面已经为“按设备路径挂接 CXL 内存”预留了配置入口。
- 基准程序里也给出了典型初始化方式，例如把 `/dev/dax1.0` 映射成 4 GiB 的 CXL 池。

### 3. 构建与依赖支持

- 构建脚本已经链接 `-lmemkind`。
- `src/Makefrag`、`src/MakefragClient` 已把 `MemoryManager.cc` 纳入编译。
- 说明 CXL 支持不是外部脚本实验，而是已经进入主工程构建链路。

### 4. 共享内存/超节点实验准备

- 仓库提供了 `prepare_shm.sh`，可在 `/dev/shm` 中预先准备一块共享内存文件。
- 项目脚本里也反复使用 `logs/shm`、`/dev/shm` 存放协作状态和进程信息。
- 这些内容更偏“实验与部署支撑”，表明该 fork 明显面向共享内存/超节点场景。

### 5. CXL 内存状态观测

- `MemoryManager::display_status()` 可以通过 `memkind_get_stat` 打印 resident、active、allocated 等统计信息。
- 这对分析 CXL 池的实际占用情况、做 benchmark 对比很有用。

## 当前集成状态判断

从现有代码看，CXL 支持已经有基础设施，但还没有全面渗透 RAMCloud 主数据路径：

- `TODO` 里明确写着“Log 从 CXL 内存中分配”，说明日志主路径仍在推进中。
- `cxl_malloc/cxl_memalign` 目前已经实现，但核心对象/日志代码里仍大量使用 `Memory::xmalloc/xmemalign`。
- 也就是说，当前最明确落地的是“CXL 池管理能力”和“可切换的分配接口”，而不是“RAMCloud 全部对象数据已迁到 CXL”。

因此，更准确的描述应是：

`ramcloud-cxl` 已经为 RAMCloud 增加了 CXL 内存池、CXL 分配 API、设备配置项、memkind 集成和实验/监控入口；但从代码状态看，核心 log/object 路径还处在逐步接入 CXL 的阶段，而不是已经彻底完成 CXL 化。
