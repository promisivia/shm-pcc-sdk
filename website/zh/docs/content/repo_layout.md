# 目录结构（Repo Layout）

下面是你最常需要关注的目录（按“从使用者视角的优先级”排序）：

## 核心与组件

- `shm-lib/`：核心库（共享内存映射、分配器、消息队列、连接/编排等基础设施）
  - `shm-lib/include/shm/**`：内存映射与分配器（`cacheable`、`CXLAllocator`、`cxl_vector` 等）
  - `shm-lib/include/msg/**`：共享内存消息队列与分发/收集逻辑
  - `shm-lib/include/connection/**`：多机启动与 follower 编排（SSH 启动等）
- `ds/`：数据结构实现（例如 `ds/BwTree/`、`ds/Masstree/`、`ds/CLHT/` 等）

## 系统与基准

- `tests/YCSB-C/`：YCSB-C 集成样例与基准（经常是“怎么用组件”的最佳参考）
- `demos/`：更小的 demo（便于快速验证某个组件能否跑通）

## 应用

- `apps/`：应用/第三方集成与评测脚本（按需挑选；每个子目录通常自带 README）

## 其它

- `tests/`：正确性/性能测试集合
- `malloc/`、`stm/`：底层/事务相关模块（通常只有在你要改底层时才需要深入）
- `website/`：本文档站点（Sphinx + RTD）

```{note}
目录结构会随着实现推进变化；如果文档与仓库不一致，以仓库为准，并欢迎提 issue/PR 修正文档。
```
