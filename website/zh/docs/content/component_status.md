# 组件与状态（Component Matrix）

本页用最短的信息告诉你“现在能用什么、缺什么、下一步会补什么”。

## 数据结构（Data Structures）

- 已接入 YCSB-C 适配的索引/哈希（可直接跑基准）：
  - Tree/Index：BwTree、Masstree、RadixART、BTree(OLC)
  - Hash：CLHT、ClevelHash、HOT 等（以 `tests/YCSB-C/` 的 `-db` 选项为准）
- 其它容器形态（例如更通用的 `map/list`）：**按模块推进**（以仓库实现与 build 选项为准）

## RPC / 通信（RPC / Comms）

- 共享内存消息队列（队列/分发/收集）：**可用**（`shm-lib/include/msg/**`）
- 多机编排/协作（SSH 启动 follower）：**有样例**（YCSB-C 的 master/follower 逻辑）
- “语义增强的 RPC”（理解数据结构操作意图的通信层）：**持续演进中**

## 应用与场景（Apps）

- YCSB-C：**可作为主样例/基准**
- 其它应用（STAMP、图计算/微服务等集成）：**按目录推进**（`apps/**`）

```{note}
如果你准备基于某个组件做集成，建议先从对应的“组件使用文档”进入，再回到这里确认状态与限制。
```
