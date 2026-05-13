# Shared CXL Pool for RAMCloud

## 目标

本次修改的目标是让多个 RAMCloud server 实例在同一台机器上启动时，共享同一个 CXL 内存池进行分配，而不是每个实例各自使用本地匿名内存，或者把共享文件预先切成固定分区。

这里的“共享”指：

- 所有 server 都指向同一个 CXL backing file，例如 `/dev/shm/ramcloud_cxl_pool`
- 分配元数据也位于这个共享文件中
- 各个 server 通过共享池头部的原子变量协调分配
- 不再依赖 `slot` 这种“预切片”方式

当前实现的范围是：

- 把 RAMCloud master log/seglet 的大块内存改为从共享 CXL 池中动态保留
- 支持本机启动 4 个 RAMCloud 实例共享同一个池
- 分配器当前是全局共享的 bump allocator

当前还没有实现：

- 通用 free/reuse 回收
- 更复杂的 free list / page queue / bitmap allocator
- 细粒度对象级共享 allocator

## 设计演进

### 当前方案：共享池头部 + 原子 bump allocator

当前实现采用：

- 共享文件起始位置存放一个 `SharedPoolHeader`
- 头部中有一个全局原子变量 `nextFreeOffset`
- 每个 server 启动时，`SegletAllocator` 通过 `LargeBlockOfMemory` 向共享池申请一整块连续空间
- 申请使用 `compare_exchange_weak` 对 `nextFreeOffset` 做原子推进

这样所有 server：

- 看见的是同一个池
- 通过同一个共享头部元数据进行协调
- 不再需要静态切片

## 当前内存分配路径

RAMCloud 里这次真正接入共享池的是 master log/seglet 大块内存，而不是零散的小对象分配。

关键路径：

1. `ServerMain` 解析 `--cxlMemoryPath` 和 `--cxlMemorySize`
2. `ServerConfig` 持有这些配置
3. `SegletAllocator` 构造时，如果发现启用了 CXL pool
4. `SegletAllocator` 通过 `LargeBlockOfMemory` 的共享池构造函数
5. 从共享池原子保留一块连续空间
6. 这块空间再被切成 seglet，供 RAMCloud log 使用

也就是说，现在共享 CXL 池已经接到 RAMCloud 真正的大块日志内存上了。

## 具体修改了什么

### 1. `src/ServerMain.cc`

文件：

- [src/ServerMain.cc](/home/wfn/pccshm-sdk/ramcloud-cxl/src/ServerMain.cc:142)

修改内容：

- 新增命令行参数：
  - `--cxlMemoryPath`
  - `--cxlMemorySize`
  - `--cxlPoolSlots`
  - `--cxlPoolSlot`
- 其中前两个是当前共享池实际使用的参数
- 后两个保留为兼容字段，但当前共享池模式里已经不再使用，只打印为 deprecated

作用：

- 允许 RAMCloud server 直接通过命令行接入共享 CXL pool

### 2. `src/ServerConfig.h`

文件：

- [src/ServerConfig.h](/home/wfn/pccshm-sdk/ramcloud-cxl/src/ServerConfig.h:228)

修改内容：

- 在 `ServerConfig` 中新增字段：
  - `string cxlMemoryPath;`
  - `size_t cxlMemorySize;`
  - `uint32_t cxlPoolSlots;`
  - `uint32_t cxlPoolSlot;`
- 并在 testing/execution 构造函数中加入默认值

作用：

- 把共享池配置从命令行传递到 `SegletAllocator`

### 3. `src/SegletAllocator.cc`

文件：

- [src/SegletAllocator.cc](/home/wfn/pccshm-sdk/ramcloud-cxl/src/SegletAllocator.cc:37)

修改内容：

- 修改 `SegletAllocator` 的 `block(...)` 初始化方式
- 原本始终是：
  - `LargeBlockOfMemory<uint8_t>(config->master.logBytes, config->master.useHugepages)`
- 现在变为：
  - 如果没有配置 `cxlMemoryPath/cxlMemorySize`，继续走原来的匿名大块内存
  - 如果配置了共享池，则改为：
    - `LargeBlockOfMemory<uint8_t>(config->cxlMemoryPath, config->master.logBytes, true)`

作用：

- 把 RAMCloud master log 使用的 seglet backing memory 接到共享 CXL pool

这是这次最关键的接入点。

### 4. `src/LargeBlockOfMemory.h`

文件：

- [src/LargeBlockOfMemory.h](/home/wfn/pccshm-sdk/ramcloud-cxl/src/LargeBlockOfMemory.h:42)

修改内容分几部分。

#### 4.1 新增共享池头部定义

在 `LargeBlockOfMemoryInternal` 中新增：

- `SHARED_POOL_MAGIC`
- `SHARED_POOL_VERSION`
- `SharedPoolHeader`

`SharedPoolHeader` 包含：

- `magic`
- `version`
- `totalBytes`
- `reservedBytes`
- `std::atomic<uint64_t> nextFreeOffset`

作用：

- 所有进程共享同一份 allocator 元数据

#### 4.2 新增共享池构造函数

新增构造函数：

- `LargeBlockOfMemory(string filePath, size_t length, bool sharedPool)`

这个构造函数会：

1. 打开共享池文件
2. `fstat` 获取总大小
3. `mmap` 文件头
4. 用 `flock` 保证首次初始化 header 时不会竞争
5. 如果 header 还没初始化：
   - 写入 magic/version
   - 写入 totalBytes
   - 设置 `nextFreeOffset = headerBytes`
6. 解锁
7. 用 `compare_exchange_weak` 原子推进 `nextFreeOffset`
8. 得到本次分配的 offset
9. 再把对应 offset 处的长度 `length` 映射到当前进程地址空间

作用：

- 实现“多个 server 真正从一个全局共享池动态取一块内存”

#### 4.3 调整原有 file-backed mmap 代码

原有通过文件映射的构造函数内部调用：

- `mmapGigabyteAligned(length, 0, fd, 0)`

并且 `mmapGigabyteAligned` 新增了 `offset` 参数，支持从共享文件的指定偏移处映射。

作用：

- 共享池保留了一块 offset 以后，可以把这个 offset 上的区域映射进来

#### 4.4 新增 move 语义

新增：

- move constructor
- move assignment

原因：

- `LargeBlockOfMemory` 原本禁用了 copy
- 在初始化列表里按值返回临时对象时会编译失败
- 加 move 以后，`SegletAllocator` 可以安全接收构造出的临时 `LargeBlockOfMemory`

### 5. `scripts/run_local_shared_cxl_4.sh`

文件：

- [scripts/run_local_shared_cxl_4.sh](/home/wfn/pccshm-sdk/ramcloud-cxl/scripts/run_local_shared_cxl_4.sh:1)

新增脚本作用：

- 本地启动 1 个 coordinator + 4 个 server
- 所有 server 共用同一个共享池文件
- 默认使用 loopback TCP，便于在一台机器上直接跑

具体行为：

- 自动选择 `obj.master/` 或 `obj/` 作为二进制目录
- 如果二进制不存在，尝试构建：
  - `make -C "$ROOT_DIR" -j2 "$OBJ_DIR/coordinator" "$OBJ_DIR/server"`
- 清理旧进程
- 重建共享池文件：
  - 默认 `/dev/shm/ramcloud_cxl_pool`
  - 默认大小 4GB
- 启动 coordinator
- 启动 4 个 server
- 每个 server 都传相同的：
  - `--cxlMemoryPath "$POOL_PATH"`
  - `--cxlMemorySize "$POOL_TOTAL_SIZE_BYTES"`

这和旧版 `slot` 脚本的区别是：

- 不再传 `--cxlPoolSlots`
- 不再传 `--cxlPoolSlot`
- 不再做固定切片

### 6. `scripts/stop_local_shared_cxl_4.sh`

文件：

- [scripts/stop_local_shared_cxl_4.sh](/home/wfn/pccshm-sdk/ramcloud-cxl/scripts/stop_local_shared_cxl_4.sh:1)

新增脚本作用：

- 清理 4 个 server 和 coordinator 的 pid
- 删除共享池文件

## 脚本使用方式

启动：

```bash
cd /home/wfn/pccshm-sdk/ramcloud-cxl
./scripts/run_local_shared_cxl_4.sh
```

停止：

```bash
cd /home/wfn/pccshm-sdk/ramcloud-cxl
./scripts/stop_local_shared_cxl_4.sh
```

可选环境变量：

- `POOL_PATH`
- `POOL_TOTAL_SIZE_MB`
- `COORD_PORT`
- `SERVER_BASE_PORT`
- `MASTER_MEMORY_MB`
- `HASHTABLE_MEMORY_MB`
- `OBJ_DIR`
- `RUN_DIR`

例如：

```bash
POOL_TOTAL_SIZE_MB=8192 MASTER_MEMORY_MB=1024 ./scripts/run_local_shared_cxl_4.sh
```

## 当前实现的边界

### 已实现

- 4 个 RAMCloud server 使用同一个共享池文件
- 分配通过共享头部的原子 bump pointer 协调
- 不再按 server 静态切片

### 未实现

- 释放后的全局复用
- 共享 free list / slab / bitmap
- 对 crash-recovery 后 allocator 状态的完整恢复
- 跨机器真实硬件环境上的一致性验证

### 目前更准确的表述

当前已经是：

- “所有 server 共享一个全局 CXL pool 动态分配”

但还不是：

- “完整通用的共享 CXL allocator”

因为现在分配是全局 bump allocator，只会向前推进，不会回收重用。

## 为什么当前实现仍然有价值

对于当前任务“起 4 个 RAMCloud 实例共享 CXL 池”来说，这个实现已经满足核心要求：

- 不是本地匿名内存
- 不是每实例单独池
- 不是静态 slot 分区
- 是所有 server 基于同一共享池头部元数据协调分配

它也为下一步继续扩展打下了基础：

- 后续可以在这个共享池头部继续加入 free list / page allocator / segment recycler
- 无需再改 RAMCloud 与共享池的接入点

## 当前验证情况

已经完成：

- 脚本语法检查：`bash -n`

未完成：

- 在当前环境中完成 `server/coordinator` 全量构建

原因：

- 仓库当前构建本身受环境影响失败
- 现有构建命令仍使用 `-std=c++11`
- 但系统中的 gtest 头文件要求 C++14
- 因此构建错误发生在工程原有代码与外部环境冲突处，不是这次共享池修改单独引入的错误

## 下一步建议

如果后续继续做，可以按这个顺序推进：

1. 给共享池增加 free/reuse 机制
2. 为 allocator 增加 crash-safe/recovery 处理
3. 增加一个最小自测程序，验证 4 个 server 从同一池拿到的区间不重叠
4. 再考虑把更多 RAMCloud 分配路径接入共享 allocator，而不仅是 seglet/log 大块内存
