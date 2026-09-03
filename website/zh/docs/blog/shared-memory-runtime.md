<div class="post-kicker">01 / Runtime</div>

# shm-lib：共享内存编程模型

<div class="post-meta"><span>状态：待展开</span><span>核心目录：shm-lib/</span></div>

## 我们想讲清楚的问题

- 为什么共享一段内存还不够，还需要一个 runtime？
- 多进程如何对同一个对象建立一致认知？
- 非临时指针、对象生命周期和映射地址之间有什么约束？
- 队列、helper 和 master/follower 协作属于哪一层？

## 文章骨架

1. 问题背景：从 `mmap` 到可编程的共享对象
2. 初始化与映射路径
3. 指针与对象语义
4. 多进程/多机协作
5. 错误模型与实现边界
6. 最小可运行示例与调试方法

```{seealso}
当前的 API 信息可先查看 {doc}`../api/shm-lib-api` 和 {doc}`../guides/guides-overview`。
```
