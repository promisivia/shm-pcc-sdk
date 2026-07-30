# Comms

本页把 “跨机协作/通信/调度” 相关入口集中在一起，结构对齐 NCCL 的 `api/comms.html` 阅读节奏（按功能分组）。

## Follower 进程管理（SSH）

### `connection/establish.h`

```{literalinclude} ../../../../shm-lib/include/connection/establish.h
:language: cpp
```

## 消息队列与 dispatcher 注册（模块级入口）

### `shm/mempool.h`

```{literalinclude} ../../../../shm-lib/include/shm/mempool.h
:language: cpp
```
