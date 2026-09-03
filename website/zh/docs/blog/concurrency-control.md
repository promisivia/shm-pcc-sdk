<div class="post-kicker">05 / Concurrency</div>

# 并发控制：OCC、STM 与数据结构内建机制

<div class="post-meta"><span>状态：待展开</span><span>核心目录：stm/</span></div>

## 我们想讲清楚的问题

- OCC、STM 与结构内建的 lock-free/locking 机制是互补还是替代关系？
- 冲突检测、验证和回滚的代价在哪里发生？
- 跨进程/共享内存环境会给原有并发算法带来哪些新约束？
- 怎样用 workload 特征来选择机制？

## 文章骨架

1. 仓库中并发控制机制的分类
2. OCC 的关键路径
3. TinySTM、TL2 与 SwissTM 的位置
4. 数据结构自带机制的边界
5. 一致性、进展保证与故障语义
6. 如何构造有解释力的对比实验
