# 仓库组件一览

按目录梳理 SDK 当前包含的主要模块，便于你定位“该看哪里/改哪里/复用哪里”。

## Core Library

- `shm-lib/`：核心库（allocator、队列、跨机协作相关基础设施）

你要接入共享内存分配/容器/队列，基本都会从这里开始：`shm-lib/include/**`。

## Benchmarks / Systems

- `tests/YCSB-C/`：YCSB-C（集成 shm-lib 的真实使用样例；也是文档示例的主要来源）
- `ds/BwTree/`：BwTree（含对 `cacheable` 分配器的接入路径）

建议把 `tests/YCSB-C/ycsbc.cc` 视为“最权威的接入样例”，其中包含 allocator 初始化、对象构造与多机启动等关键路径。

## Demos / Apps

- `demos/`：小型 demo（例如初始化/分配/数据结构 demo）
- `apps/`：若干应用/第三方集成（按需使用）

## Others

- `malloc/`：分配器/相关实验
- `stm/`：事务/并发相关模块
- `tests/`：正确性/性能测试集合
