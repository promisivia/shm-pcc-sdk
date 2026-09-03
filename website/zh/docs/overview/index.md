# 概览

CXL-SDK 的目标是降低共享内存与 CXL/UB 系统原型的开发成本。仓库将常见能力拆成可组合的模块，让应用不必从内存映射、跨进程对象和并发容器的底层细节重新开始。

## 整体功能

- **共享内存运行时**：封装内存映射、共享对象生命周期、分配器与非临时指针。
- **并发数据结构**：提供 BwTree、Masstree、CLHT 等结构及统一的评测入口。
- **通信与协作**：包含共享内存消息队列、RPC，以及多进程和多机协作路径。
- **应用与评测**：通过 YCSB-C 和仓库内应用验证正确性与性能。

更深入的模块说明见 {doc}`../content/architecture`、{doc}`../content/tech_route` 和 {doc}`../components/index`。

## 安装与使用

从 {doc}`../user-guide` 开始完成依赖安装、构建和第一个 YCSB-C 工作负载。运行环境相关的参数集中在 {doc}`../environment-variables`。

## Roadmap

当前进展和后续方向见 {doc}`../roadmap`。Roadmap 会随着实现与验证结果持续调整。
