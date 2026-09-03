---
orphan: true
---

<div class="post-kicker">02 / Memory</div>

# 分配器：从字节到可共享对象

<div class="post-meta"><span>状态：待展开</span><span>核心目录：malloc/</span></div>

## 我们想讲清楚的问题

- 共享内存分配与普通 heap 分配的根本差异是什么？
- memkind、lsmalloc 与 cxlalloc 在仓库中各自承担什么角色？
- 固定映射地址、offset pointer 和对象恢复分别会带来什么取舍？
- 分配器的性能应该怎么测，才不会被 workload 误导？

## 文章骨架

1. 内存区域与地址空间
2. 分配 API 与后端抽象
3. 元数据、并发与回收
4. 持久性/恢复语义
5. 三类后端的比较
6. 评测指标与常见陷阱

```{seealso}
开发者指南：{doc}`../guides/memory-allocation`。
```
