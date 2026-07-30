# 通信 / RPC（Msg Queue & Multi-machine）

本仓库里“RPC/通信”更贴近两类能力：

1. **共享内存消息队列**：把请求/任务在多线程（以及可扩展到多进程）之间传递与调度
2. **多机编排（master/follower）**：通过 SSH 启动 follower，按统一参数协同运行（YCSB-C 作为主要参考）

如果你在找“传统的 TCP/gRPC 风格 RPC”，建议先从这里理解仓库现有的通信基础设施，再决定是否需要引入外部 RPC 框架。

## 1. 共享内存消息队列（MsgQueue）

消息队列位于 `shm-lib/include/msg/**`。其中 `MsgQueue` 定义了队列页布局与入队/出队接口：

```{literalinclude} ../../../../shm-lib/include/msg/msg_queue.h
:language: c++
```

配套的 `MsgDispatcher`/`MsgCollector` 提供了“按消息类型分发处理”的线程模型：

```{literalinclude} ../../../../shm-lib/include/msg/msg_dispatcher.h
:language: c++
```

## 2. 在 YCSB-C 中启用消息队列路径（`USE_MSG_QUEUE`）

YCSB-C 的 CMake 会为不同 variant 选择不同的编译宏；例如 `*_mq` 变体会定义 `USE_MSG_QUEUE`（见 `tests/YCSB-C/CMakeLists.txt` 的可执行目标列表）。

启用后，部分 DB 适配器会使用线程池/队列把操作作为任务分发（例如 `BwTreeDB` 在 `USE_MSG_QUEUE` 下会创建 `ThreadPool` 并 enqueue 任务）。

```{note}
消息队列路径属于“把请求分发到 worker 处理”的一种实现方式；它并不等价于跨机 RPC，但它是仓库里通信/调度的核心构件之一。
```

## 3. 多机 master/follower 编排（FollowerManager）

仓库提供了基于 SSH 的 follower 启动与管理逻辑（`FollowerManager`），常用于“一主多从”的基准/实验模式：

```{literalinclude} ../../../../shm-lib/include/connection/establish.h
:language: c++
:start-after: class FollowerManager
:end-before: private:
```

YCSB-C 的 master/follower 入口在 `tests/YCSB-C/ycsbc.cc` 中，关键参数包括 `-machinenum` 与 `-follower_list`：

```{literalinclude} ../../../../tests/YCSB-C/run_shm_ds.sh
:language: bash
:start-after: run_ycsbc() {
:end-before: run_real() {
```

## 4. 把这套通信能力用到你的服务里

实践中常见的落地方式是：

- **单机多线程**：用 `MsgQueue + Dispatcher/Collector` 把“业务请求”拆成任务，交给 worker 线程处理
- **多机协作实验**：沿用 YCSB-C 的 master/follower 启动方式，确保所有节点以一致参数启动并共享同一套共享内存映射约束

当你开始做更复杂的跨机语义（例如“按 key/范围操作共享数据结构”）时，可以把消息体从“字节数组”逐步演进为“对数据结构操作的描述”，并把序列化/一致性策略下沉到组件层。

