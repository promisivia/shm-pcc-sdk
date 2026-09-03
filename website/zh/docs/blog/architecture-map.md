<div class="post-kicker">00 / Architecture</div>

# 先画地图：这个仓库想解决什么？

<div class="post-meta"><span>状态：结构稿</span><span>预计阅读：12 分钟</span><span>系列起点</span></div>

## 本篇要回答的问题

- CXL-SDK 对“共享内存系统开发”做了怎样的拆分？
- 应用、数据结构、`shm-lib` 与分配器之间如何连接？
- 第一次读这个仓库，哪条路线最容易建立完整理解？

## 一句话解释

CXL-SDK 把系统的共享状态放进具有语义的并发数据结构，用 `shm-lib` 封装映射、对象、队列与协作机制，再由可替换的分配后端承接 CXL/UB 设备或文件支撑的共享内存。

## 分层地图

```text
应用 / Benchmark        YCSB-C、STAMP、实际应用与 demos
          ↓
数据结构             BwTree、Masstree、CLHT、HOT、RadixART ...
          ↓
共享内存运行时         shm-lib：映射、对象、队列、跨进程协作
          ↓
内存管理               memkind、lsmalloc、cxlalloc 等后端
          ↓
平台                   CXL / UB 内存或文件支撑区域
```

## 建议的读代码顺序

1. 从 `demos/` 看最小完整路径，先理解“怎么用”。
2. 进入 `tests/YCSB-C/`，看一个真实 workload 怎样组装数据结构与运行时。
3. 沿着调用进入 `shm-lib/include/`，建立核心 API 的语义模型。
4. 再按问题进入 `ds/`、`malloc/`、`stm/` 和 `eRPC-LRPC/`，避免一开始陷入实现细节。

## 下一步要补的内容

```{note}
下一轮会结合真实调用链，补一张带文件入口的架构图，并把“稳定核心”与“实验性组件”区分开。
```

**继续阅读：** {doc}`shared-memory-runtime`
