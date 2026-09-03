---
orphan: true
---

<div class="post-kicker">03 / Data Structures</div>

# 数据结构：为什么它是 SDK 的中心层

<div class="post-meta"><span>状态：待展开</span><span>核心目录：ds/</span></div>

## 我们想讲清楚的问题

- 为什么不把 SDK 做成一组裸内存 API，而要把数据结构放在中心？
- BwTree、Masstree、CLHT、HOT 等实现如何适配统一 workload？
- 内存布局、访存局部性与并发算法如何共同决定性能？
- 怎样区分“算法本身”与“接入共享内存后的改造”？

## 文章骨架

1. 数据结构层在整体架构中的位置
2. 统一接口与 YCSB adapter
3. Tree / trie / hash 三类结构的对比
4. 分配、指针与回收的改造点
5. 正确性验证与性能解释
6. 新增一个数据结构的最短路径

```{seealso}
使用文档：{doc}`../components/data_structures`。
```
