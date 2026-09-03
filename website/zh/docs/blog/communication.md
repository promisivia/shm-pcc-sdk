---
orphan: true
---

<div class="post-kicker">04 / Communication</div>

# 通信：共享内存与 RPC 怎么分工

<div class="post-meta"><span>状态：待展开</span><span>关联目录：shm-lib/ · eRPC-LRPC/</span></div>

## 我们想讲清楚的问题

- 已经能共享内存，为什么还需要消息和 RPC？
- 数据面与控制面如何分工？
- 消息队列与 master/follower 机制保证了哪些语义？
- eRPC-LRPC 的路径如何与 SDK 其它部分衔接？

## 文章骨架

1. 共享访存与显式消息的能力边界
2. 队列的数据结构与同步语义
3. 跨进程和跨机协作路径
4. RPC 接入与嵌套调用
5. 延迟、吞吐与 CPU 开销
6. 故障与超时模型

```{seealso}
使用文档：{doc}`../components/rpc_comms`；开发者指南：{doc}`../guides/message-queue`。
```
