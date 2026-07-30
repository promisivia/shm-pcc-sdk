# RAMCloud

本文档从代码出发总结 RAMCloud 本身的架构，重点回答三件事：

- 系统由哪些核心模块组成。
- 每个模块负责什么。
- 这些职责在代码里对应到哪里。

code repo: https://github.com/PlatformLab/RAMCloud  
doc: https://ramcloud.atlassian.net/wiki/spaces/RAM/overview

## 1. 先看整体：RAMCloud 的代码分层

从源码看，RAMCloud 不是“一个大 KV 类”，而是几层明确分开的系统：

```text
Client API
  -> RamCloud / RpcWrapper / ObjectFinder
  -> Transport / Session / Dispatch

Coordinator control plane
  -> CoordinatorService
  -> CoordinatorServerList
  -> TableManager
  -> MasterRecoveryManager

Storage server data plane
  -> Server
  -> MasterService / BackupService / AdminService

Master internals
  -> ObjectManager
  -> TabletManager
  -> TransactionManager
  -> TxRecoveryManager
  -> UnackedRpcResults
  -> MasterTableMetadata

Log-structured storage core
  -> Log
  -> SegmentManager
  -> LogCleaner
  -> ReplicaManager
```

这套结构非常典型地体现了 RAMCloud 的设计目标：

- Coordinator 只负责元数据和恢复编排，不走对象读写数据面。
- Master 负责对象的实际读写、事务、索引和线性一致性。
- Backup 负责 segment 副本持久化和恢复数据供应。
- 真正的数据组织核心不是 “B+Tree” 或 “LSM”，而是内存中的 log-structured storage。

## 2. 进程启动与对象装配

### 2.1 共享上下文：`Context`

RAMCloud 几乎所有核心对象都挂在 `Context` 上，`Context` 更像“进程级依赖注入容器”。

对应代码：

- `src/Context.h`
- `src/Context.cc`

`Context` 里最关键的成员有：

- `dispatch`: 事件循环核心。
- `transportManager`: 负责 transport 初始化和 session 获取。
- `coordinatorSession`: 到 coordinator 的长期会话。
- `objectFinder`: 客户端和 master 用它做 tablet -> server 的查找。
- `workerManager`: 服务端 worker 线程池。
- `services[]`: 当前进程里注册的 `Service` 数组。
- `serverList / coordinatorServerList / tableManager / recoveryManager`: 只在 server 或 coordinator 角色下有效。

代码层面的重点不是“保存全局变量”，而是：

- 允许同一进程里同时存在多个 RAMCloud context。
- 明确构造/析构顺序，避免 transport、session、service 之间出现生命周期错误。

### 2.2 Storage server 启动：`ServerMain -> Server`

对应代码：

- `src/ServerMain.cc`
- `src/Server.h`
- `src/Server.cc`

`ServerMain.cc` 做的事情主要是：

- 解析命令行参数到 `ServerConfig`
- 创建 `Context`
- 初始化 transport
- 创建 `Server`
- 调 `Server::run()`

`Server` 是 master/backup/admin 的装配器，不负责 coordinator。它做两件核心工作：

1. `createAndRegisterServices()`
   - 按 `ServerConfig.services` 创建 `MasterService`、`BackupService`、`AdminService`
2. `enlist()`
   - 向 coordinator 注册
   - 取得 `ServerId`
   - 把 `ServerId` 下发给各 service
   - 启动 failure detector

因此，`Server` 的角色更接近：

- “节点生命周期管理器”
- 不是业务模块本身

### 2.3 Coordinator 启动：`CoordinatorMain`

对应代码：

- `src/CoordinatorMain.cc`

Coordinator 进程直接手工装配：

- `Context`
- `ExternalStorage`
- `CoordinatorService`
- `AdminService`

然后进入 `dispatch->poll()` 循环。

这说明 coordinator 是单独的控制平面进程，不复用 `Server` 这套 master/backup 启动器。

## 3. Coordinator 侧架构

Coordinator 的职责是“集群元数据和成员关系的唯一真相”，不在对象读写主路径上。

### 3.1 `CoordinatorService`

对应代码：

- `src/CoordinatorService.h`
- `src/CoordinatorService.cc`

它是 coordinator 的 RPC 入口，负责处理：

- `createTable/dropTable`
- `createIndex/dropIndex`
- `enlistServer`
- `getServerList`
- `getTableConfig`
- `hintServerCrashed`
- `reassignTabletOwnership`
- `splitTablet`
- lease 相关 RPC
- recovery 相关 RPC

可以把它理解成控制平面的总调度器。

### 3.2 `CoordinatorServerList`

对应代码：

- `src/CoordinatorServerList.h`
- `src/CoordinatorServerList.cc`

职责：

- 分配 `ServerId`
- 维护集群成员列表
- 跟踪 server 状态：UP、CRASHED、REMOVE 等
- 异步向集群各 server 推送 server list 增量更新
- 为 recovery 保存每个 server 的协调状态

从代码上看，它不只是一个 map，而是“带版本传播协议的成员管理器”。

关键点：

- coordinator 维护 authoritative membership
- 其他 server 本地拿到的是它同步下去的副本

### 3.3 `TableManager`

对应代码：

- `src/TableManager.h`
- `src/TableManager.cc`

职责：

- 维护 table -> tablets -> owner master 的映射
- 创建/删除 table
- split tablet
- reassign tablet ownership
- serialize table config 给 client/master
- 管理 secondary index 对应的 indexlet 和 backing table

这是 RAMCloud 控制平面最核心的元数据模块之一。  
客户端找对象时，最终依赖的就是 coordinator 这边产出的 table/tablet 配置。

### 3.4 `MasterRecoveryManager`

对应代码：

- `src/MasterRecoveryManager.h`
- `src/MasterRecoveryManager.cc`

职责：

- 当 master crash 时，协调恢复流程
- 选择恢复参与者
- 分配恢复工作
- 跟踪恢复完成

它不负责真正 replay 对象，而是负责 orchestrate recovery。

## 4. Storage server 侧架构

一个普通存储节点通常至少会跑：

- `MasterService`
- `BackupService`
- `AdminService`

### 4.1 `MasterService`

对应代码：

- `src/MasterService.h`
- `src/MasterService.cc`

这是对象访问的数据面入口。所有对象相关 RPC 基本都在这里分发：

- `read`
- `write`
- `remove`
- `increment`
- `multiRead/multiWrite/multiRemove`
- `migrateTablet`
- transaction prepare / decision
- index 相关 RPC

`MasterService` 本身不是对象存储实现，它更像 RPC facade。  
它把请求翻译成下面这些内部模块操作：

- `ObjectManager`: 真正对象存储
- `TabletManager`: tablet ownership 检查
- `TransactionManager`
- `TxRecoveryManager`
- `UnackedRpcResults`
- `IndexletManager`
- `MasterTableMetadata`

也就是说：

- `MasterService` 负责“协议层和调用编排”
- `ObjectManager` 负责“对象落盘/落日志/一致性实现”

### 4.2 `BackupService`

对应代码：

- `src/BackupService.h`
- `src/BackupService.cc`
- `src/BackupStorage.h`
- `src/BackupMasterRecovery.h`

职责：

- 接收 master 发来的 segment 副本写入
- 管理 backup 端的 replica 存储
- 在恢复时把 recovery data 提供给恢复 master
- 在 server crash 后清理老 replica

它的核心不是“对象级备份”，而是“segment 级副本管理”。  
这和 RAMCloud 的 log-structured 设计完全一致：备份的单位是 segment，不是单对象。

### 4.3 `AdminService`

对应代码：

- `src/AdminService.h`
- `src/AdminService.cc`

职责相对简单：

- 提供 server control、统计、调试、配置类 RPC

它不是数据主路径的一部分。

## 5. Master 内部模块

### 5.1 `ObjectManager`: master 真正的对象存储核心

对应代码：

- `src/ObjectManager.h`
- `src/ObjectManager.cc`

这是 master 内部最重要的模块。它把以下几样东西整合到一起：

- `Log`
- `HashTable`
- `TabletManager`
- `ReplicaManager`
- `TransactionManager`
- `TxRecoveryManager`
- `UnackedRpcResults`

头文件注释里写得很直接：  
`ObjectManager is essentially the union of the Log, HashTable, TabletMap, and ReplicaManager classes.`

它负责的核心操作包括：

- `readObject`
- `writeObject`
- `removeObject`
- `prepareOp / commitWrite / commitRemove`
- `replaySegment`
- `syncChanges`

`writeObject()` 的关键逻辑很能代表 RAMCloud 的存储思想：

1. 根据 key 找 hash bucket 并加锁
2. 用 `TabletManager` 检查当前 master 是否拥有该 tablet
3. 检查事务锁
4. 查当前对象版本
5. 计算新版本号
6. 组装 object / tombstone / rpcResult 等 log entry
7. append 到 log
8. 更新 hash table 引用

所以 ObjectManager 不只是“对象表”，而是：

- 对象可见性的执行者
- log entry 生命周期的协调者
- 内存中对象索引与日志引用之间的桥梁

### 5.2 `TabletManager`: master 本地 tablet ownership

对应代码：

- `src/TabletManager.h`
- `src/TabletManager.cc`

职责：

- 管理本 master 当前拥有的 tablet 范围
- 判断某个 key 是否属于当前 master
- 记录 tablet 的 `NORMAL / NOT_READY / LOCKED_FOR_MIGRATION` 状态
- 统计 tablet 读写计数

它和 coordinator 的 `TableManager` 不是一个层级：

- `TableManager` 是全局真相
- `TabletManager` 是单 master 上的本地 ownership 视图

### 5.3 `TransactionManager`

对应代码：

- `src/TransactionManager.h`
- `src/TransactionManager.cc`

职责：

- 管理事务中的 `PreparedOp` 和 `ParticipantList`
- 为未完成事务设置超时和恢复触发
- 保证事务相关的日志记录在需要时不被提前清理
- 在恢复后重新抓取必要的对象锁

它的设计不是完整的事务执行器，而是：

- 管 server 侧 prepared 状态
- 管事务恢复所需的持久状态

### 5.4 `TxRecoveryManager`

对应代码：

- `src/TxRecoveryManager.h`
- `src/TxRecoveryManager.cc`

职责：

- 跟踪正在执行的 transaction recovery
- 在恢复过程中发起 RPC、汇总投票、推进事务决议

可以看成事务故障恢复专用协调模块。

### 5.5 `UnackedRpcResults`

对应代码：

- `src/UnackedRpcResults.h`
- `src/UnackedRpcResults.cc`

职责：

- 记录尚未被 client ack 的线性一致 RPC 结果
- 检测重复 RPC
- 防止 client retry 导致同一线性化请求被重复执行
- 为事务恢复保留必要 RPC result

这是 RAMCloud 实现线性一致 client RPC 的关键模块之一。  
它解释了为什么 `write/read-modify-write/transaction prepare` 这类 RPC 可以在 retry 下仍保持语义正确。

### 5.6 `MasterTableMetadata`

对应代码：

- `src/MasterTableMetadata.h`
- `src/MasterTableMetadata.cc`

职责：

- 保存每个 table 在本 master 上的附加元数据
- 给 `TableStats` 等子模块提供按 table 存储 block 的容器

它不是对象主路径模块，但很多统计和管理逻辑依赖它。

## 6. 存储内核：为什么 RAMCloud 是 log-structured

### 6.1 `Log`

对应代码：

- `src/Log.h`
- `src/Log.cc`

`Log` 是 master 内存存储的核心抽象：

- 数据不是原地更新
- 而是把 object / tombstone / rpc result / prepared op 等 entry 追加到 log
- 真正 durability 依赖把这些 segment 副本推到 backup

核心方法：

- `append`
- `sync`
- `syncTo`
- `rollHeadOver`

重点：

- append 不等于 durable
- 调 `sync()` 后才保证之前追加的数据已经复制到 backups

### 6.2 `SegmentManager`

对应代码：

- `src/SegmentManager.h`
- `src/SegmentManager.cc`

职责：

- 管理 log 使用的 segment 生命周期
- 分配 head segment / side segment
- 维护 segment state machine
- 控制哪些 segment 可清理、可释放、可重用
- 在 cleaner 和正常写入之间协调 segment 资源

这是 log 底层的内存资源管理器。  
`Log` 比较偏“追加语义”，`SegmentManager` 偏“segment 生命周期与资源调度”。

### 6.3 `LogCleaner`

对应代码：

- `src/LogCleaner.h`
- `src/LogCleaner.cc`

职责：

- 清理 closed segments 中的死数据
- 复制 live entries 到 survivor segments
- 回收被 tombstone 和旧版本对象浪费的空间
- 控制内存清理和磁盘清理的平衡

这是 RAMCloud 可以长期运行、不被 append-only 日志耗尽内存的关键后台模块。

### 6.4 `ReplicaManager`

对应代码：

- `src/ReplicaManager.h`
- `src/ReplicaManager.cc`

职责：

- 把 master log segment 复制到 backup
- 管理 replica 放置和同步
- 与 log / cleaner / recovery 协同

它连接了 master 的内存 log 和 backup 的持久副本。

## 7. 客户端路径：对象是怎样定位到 master 的

### 7.1 `RamCloud`

对应代码：

- `src/RamCloud.h`
- `src/RamCloud.cc`

这是用户态 client API。

它暴露的接口包括：

- `createTable`
- `read`
- `write`
- `remove`
- `multi*`
- `increment`
- `migrateTablet`

但这些 API 自己不处理路由，它们大多只是：

- 构造对应的 `RpcWrapper`
- 等待 RPC 完成

### 7.2 `ObjectFinder`

对应代码：

- `src/ObjectFinder.h`
- `src/ObjectFinder.cc`

职责：

- 根据 `(tableId, keyHash)` 查出目标 tablet
- 根据 tablet 找到目标 master 的 service locator / session
- 缓存 table config
- 在 tablet 迁移或 server 出错时刷新缓存

这是 client 到 master 路由的关键模块。

它的缓存对象是：

- `TabletWithLocator`
- `IndexletWithLocator`

也就是说，client 并不直接知道“哪个 server 管哪个 key”，而是通过 object finder 懒加载 coordinator 的 table config。

### 7.3 `RpcWrapper / Transport`

对应代码：

- `src/RpcWrapper.h`
- `src/ObjectRpcWrapper.cc`
- `src/CoordinatorRpcWrapper.cc`
- `src/Transport.h`
- `src/TransportManager.h`

职责：

- 统一封装客户端异步 RPC 生命周期
- 处理 retry、transport error、session 复用
- 通过 transport 实际发送 request buffer

对对象 RPC 来说，关键路径在 `ObjectRpcWrapper::send()`：

1. 通过 `context->objectFinder->tryLookup(tableId, keyHash)` 找 session
2. 调 `session->sendRequest(...)` 发往 master

所以对象路由真正发生的位置不是 `RamCloud::write()`，而是 `ObjectRpcWrapper + ObjectFinder`。

## 8. 关键调用链：以 write 为例

RAMCloud 的写路径很适合用来理解整个架构怎么协同。

### 8.1 客户端侧

对应代码：

- `src/RamCloud.cc`
- `src/ObjectRpcWrapper.cc`

调用链：

```text
RamCloud::write()
  -> WriteRpc::WriteRpc()
  -> ObjectRpcWrapper::send()
  -> ObjectFinder::tryLookup()
  -> session->sendRequest()
```

这里各层的职责分别是：

- `RamCloud::write`: 暴露 API
- `WriteRpc`: 组装 WireFormat::Write 请求和对象数据 buffer
- `ObjectRpcWrapper`: 统一对象 RPC 的路由和重试逻辑
- `ObjectFinder`: 找 master
- `Transport::Session`: 真的发包

### 8.2 master 侧

对应代码：

- `src/MasterService.cc`
- `src/ObjectManager.cc`

调用链：

```text
MasterService::write()
  -> UnackedRpcResults duplicate check
  -> 构造临时 Object
  -> requestInsertIndexEntries()
  -> ObjectManager::writeObject()
  -> ObjectManager::syncChanges()
  -> 记录 RpcResult
```

`MasterService::write()` 做的是协议层工作：

- 去重
- 解析 object
- 处理索引前后逻辑
- 组织线性一致 RPC result

`ObjectManager::writeObject()` 做的是存储层工作：

- tablet ownership 检查
- transaction lock 检查
- 旧版本查找
- 新版本分配
- tombstone/object/rpcResult 组装
- append 到 log
- 更新 hash table

`ObjectManager::syncChanges()` 则把前面的 append 同步到 backup，对应到底层就是 `log.sync()`。

### 8.3 这个调用链说明了什么

RAMCloud 写路径最核心的设计不是“直接修改对象”，而是：

- 先把对象变成 log entry
- 用 hash table 指向当前有效版本
- 用 tombstone 和 cleaner 管历史垃圾
- 用 backup segment 复制保证 durability
- 用 unacked rpc result 保证线性一致重试语义

这正是 RAMCloud 架构的本质。

## 9. 模块之间的关系

如果只保留最重要的依赖关系，可以概括成：

```text
CoordinatorService
  -> CoordinatorServerList
  -> TableManager
  -> MasterRecoveryManager

Server
  -> MasterService
  -> BackupService
  -> AdminService

MasterService
  -> ObjectManager
  -> TabletManager
  -> TransactionManager
  -> TxRecoveryManager
  -> UnackedRpcResults
  -> MasterTableMetadata

ObjectManager
  -> Log
  -> HashTable
  -> SegmentManager
  -> ReplicaManager
  -> TabletManager
  -> TransactionManager
  -> TxRecoveryManager

Client RamCloud
  -> RpcWrapper
  -> ObjectFinder
  -> TransportManager / Session
```

## 10. 结论

从代码角度看，RAMCloud 的架构可以浓缩成一句话：

RAMCloud 是一个“由 coordinator 维护全局 tablet 元数据、由 master 提供对象读写、由 backup 持久化 segment 副本、以 log-structured in-memory storage 为核心”的分布式对象存储。

如果再进一步压缩到最关键的几个点：

- `CoordinatorService + TableManager` 决定“谁拥有哪个 tablet”
- `ObjectFinder` 决定“客户端把请求发给谁”
- `MasterService` 决定“收到 RPC 后怎么编排”
- `ObjectManager` 决定“对象如何真正写入和可见”
- `Log + SegmentManager + LogCleaner + ReplicaManager` 决定“数据如何存、复制、回收、恢复”
- `UnackedRpcResults + TransactionManager` 决定“强一致重试和事务恢复如何成立”

如果后面要继续深入，最值得继续顺着读的源码是：

1. `src/RamCloud.cc`
2. `src/ObjectRpcWrapper.cc`
3. `src/ObjectFinder.cc`
4. `src/MasterService.cc`
5. `src/ObjectManager.cc`
6. `src/Log.cc`
7. `src/SegmentManager.cc`
8. `src/CoordinatorService.cc`
9. `src/TableManager.cc`
