---
orphan: true
---

<div class="post-kicker">06 / Evaluation</div>

# 应用与评测：怎样证明设计有效

<div class="post-meta"><span>状态：待展开</span><span>关联目录：demos/ · tests/ · apps/</span></div>

## 我们想讲清楚的问题

- demo、microbenchmark 和完整应用分别能证明什么？
- YCSB-C 的 workload、adapter 与配置如何共同影响结论？
- 没有 CXL 硬件时，文件支撑的 smoke test 有什么价值和局限？
- 怎样记录环境、基线和变量，让评测真正可复现？

## 文章骨架

1. 从功能闭环到性能结论
2. demos：验证最小路径
3. correctness tests：验证不变式
4. YCSB-C：对比数据结构与并发方案
5. applications：暴露组合成本和真实瓶颈
6. 一份可复现实验的检查清单

```{seealso}
使用文档：{doc}`../components/apps`；仓库总览：{doc}`../content/repo_components`。
```
