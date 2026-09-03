# 设计手记

<div class="blog-hero">
  <div class="blog-kicker">CXL-SDK / Engineering Notes</div>
  <h1>不只告诉你代码在哪里，还讲清楚它为什么这样设计。</h1>
  <p>这是一系列面向系统开发者的长文：从整体架构开始，逐步拆解共享内存运行时、分配器、数据结构、通信、并发控制与评测方法。</p>
</div>

## 从这里开始

<div class="blog-grid">
  <a class="blog-card" href="architecture-map.html">
    <span class="card-number">00 / OVERVIEW</span>
    <h3>先画地图：这个仓库想解决什么？</h3>
    <p>建立阅读顺序，看懂应用、数据结构、运行时和内存后端之间的关系。</p>
    <span class="card-status">已有骨架 →</span>
  </a>
  <a class="blog-card" href="shared-memory-runtime.html">
    <span class="card-number">01 / RUNTIME</span>
    <h3>shm-lib：共享内存编程模型</h3>
    <p>映射、对象生命周期、非临时指针，以及跨进程状态如何被组织起来。</p>
    <span class="card-status">待展开 →</span>
  </a>
  <a class="blog-card" href="memory-allocation.html">
    <span class="card-number">02 / MEMORY</span>
    <h3>分配器：从字节到可共享对象</h3>
    <p>分配后端、地址约束、恢复与性能之间如何取舍。</p>
    <span class="card-status">待展开 →</span>
  </a>
  <a class="blog-card" href="data-structures.html">
    <span class="card-number">03 / DATA STRUCTURES</span>
    <h3>数据结构：为什么它是 SDK 的中心层</h3>
    <p>从 BwTree、Masstree 到 CLHT，看接口适配、并发语义和内存布局。</p>
    <span class="card-status">待展开 →</span>
  </a>
  <a class="blog-card" href="communication.html">
    <span class="card-number">04 / COMMUNICATION</span>
    <h3>通信：共享内存与 RPC 怎么分工</h3>
    <p>消息队列、master/follower 协作和 eRPC-LRPC 路径中的边界设计。</p>
    <span class="card-status">待展开 →</span>
  </a>
  <a class="blog-card" href="concurrency-control.html">
    <span class="card-number">05 / CONCURRENCY</span>
    <h3>并发控制：OCC、STM 与数据结构内建机制</h3>
    <p>整理多种并发方案的适用边界，以及它们在仓库中的位置。</p>
    <span class="card-status">待展开 →</span>
  </a>
  <a class="blog-card" href="applications-and-evaluation.html">
    <span class="card-number">06 / EVALUATION</span>
    <h3>应用与评测：怎样证明设计有效</h3>
    <p>从 demo 到 YCSB-C 和完整应用，建立可复现的评估链路。</p>
    <span class="card-status">待展开 →</span>
  </a>
</div>

## 每篇文章都会回答什么

1. 这个模块面对的核心问题是什么？
2. 现有设计的主路径和关键抽象是什么？
3. 当时可能做过哪些取舍，代价是什么？
4. 如果要读代码或继续开发，应该从哪里进入？
5. 如何用 demo、测试和 benchmark 验证自己的理解？

```{toctree}
:hidden:
:maxdepth: 1

architecture-map
shared-memory-runtime
memory-allocation
data-structures
communication
concurrency-control
applications-and-evaluation
```
