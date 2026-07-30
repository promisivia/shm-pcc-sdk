# Shared memory sdk

## 应用背景

CXL/UB等共享内存兴起, 有很多应用通过共享内存zero-copy等能力获得了性能提升，
- tigon：共享内存做跨机器transction, 避免2PC开销
- CXL-SHM：构建跨机器的bypass-by-reference消息通信，避免消息传递时的网络开销和内存copy
- 我们自己探索的ray、图计算等场景也可以基于CXL获得收益

![[image-20260313085916036.png]]
## 我们想做的

以上的平台基本还是靠自己手写
提供 平台应该提供的基础编程框架还是一个问题...Tigon
## Background

CXL hardware的架构，参考[[Unified Bus]]和[[CXL]]

部分缓存一致性，
- 部分有CC，参考
- 

## Overview of 我们认为应该有的基础设施

![[image-20260120192154508.png|white]]

## 底层同步

## Hardware特征

## Library

### DS as Core

key idea: data structures (e.g., std::map, set, vector, etc., for CXL platform) is the core of the library
我们要构建一个以数据

1. Applications directly use these data structures to store and retrieve data
   -- mapreduce-based applications like spark/ray use vector/hash to share intermediate results
   -- graph processing use graph-based data structures to share graph state
   -- database use indexes (tree-based data structures) to share data
2. Several advantages:
   - memory management: enhanced memory management: SHM-PCC(sosp23) pratial failure memory management, data allocated but failed is leaked, and SHM-PCC uses counter to track the live data; if all applications uses memory from std::ds from our libraries, this problem is solved; like privous NVM allocator studies, data structures has a root, data reachable from the root are live, other allocated but not reachable data are leaked; this provide a changes for us to reclaim leaked memory; this garbage collection 完全可以少部分地去做
   - enhanced RPC
     If applications use our stranded std::ds,


### 关键技术优势

这种设计方法不仅提供了一套统一的抽象接口，还解决了共享内存系统中的几个核心挑战：

1. 高效性和正确性由数据结构统一保证

ü我们的数据结构面向部分缓存一致性平台进行设计，可以正确高效的运行

ü（来自上一项工作，不过上一项工作支持的数据结构有限，我们在进行扩展）

2. 增强的内存管理与垃圾回收：

ü针对共享内存中的部分失效（Partial Failure）和内存泄漏问题

ü该 SDK 引入了类似于 NVM 分配器的管理机制：通过定义数据结构的“根”节点，系统可以追踪所有可达数据；不可达的数据将被视为泄漏，并由系统进行小规模的垃圾回收。

![[image-20260313100928586.png]]



3. 所有基于数据开发的系统组件都可以获得以上优势

ü远程过程调用（RPC）：当使用 SDK 提供的标准数据结构时，SDK可以知道应用通信的语义，从而管理RPC共享数据


### 还需要实现的内容

1. 硬件同步

üLock/Lock-free (Old)

üTx Memory (New)

2. 数据结构

üHash/Tree数据结构 (Old)

ü更多数据结构，e.g. Map/List  (New)

p包装一键可用的std::map (DOING)

3. 系统组件

ü对象存储系统(Old), ~~支持~~~~transaction~~~~语义~~，已验证Ray上的效果

pRPC框架 (New)，结合RPC框架和数据结构 (DOING)，验证microservice/Redis等场景的效果（关键结果）

4. 内存管理

p开发底层分配器、GC策略 (DOING)

- microservices + redis
- Flink Nett：IPC