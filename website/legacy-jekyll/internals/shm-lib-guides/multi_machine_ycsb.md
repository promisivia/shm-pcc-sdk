# Multi-machine（YCSB master/follower）

YCSB-C 在多机场景里通过 `FollowerManager` 启动 follower 进程/节点，并在 master 侧分配共享变量（例如 `GlobalVariables`）供 follower 读取。

如果你要在自研系统里做类似的 “master 编排 + follower 执行”，本页用于帮你厘清：

- `FollowerManager` 负责什么、不负责什么
- 共享变量如何分配与传递（以及固定映射基址的意义）

## 你需要准备的输入（最小集合）

- follower 列表（hostnames / IP）
- 每个 follower 的启动命令构造方式（通常由 master 侧拼接参数）
- 一段共享变量的地址传递机制（YCSB 里用 `share_var` 传递指针地址字符串）

## 入口：`FollowerManager`

`shm-lib/include/connection/establish.h`：

```{literalinclude} ../../../../shm-lib/include/connection/establish.h
:language: cpp
```

## 真实用法：YCSB-C `MasterTransactionManager`

```{literalinclude} ../../../../tests/YCSB-C/include/core/delegator/master_trx_manager.h
:language: cpp
```
