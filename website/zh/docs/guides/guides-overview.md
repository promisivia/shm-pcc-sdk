# Guides 总览

本页给出面向SDK开发者的文档，给出 `shm-lib` 开发应用的常见接口与使用方法。

## 适用场景

适用场景：
- 你要开发一个新的数据结构
- 你要修复当前SDK的bug、新增功能、性能优化等
不适用场景：
- 你要直接使用SDK开发应用，而不是开发SDK

## 推荐阅读顺序

1. {doc}`memory-allocation`：先确保分配器初始化与对象生命周期正确  
2. {doc}`nt-pointer`：再统一 `nt<T>`/`nt_pointer<T>` 访问语义  
3. {doc}`message-queue`：然后接入消息分发与跨机回收  
4. {doc}`multi-process`：最后做多进程/多机联调与回归验证

## 两个参考实现

### YCSB-C：共享内存初始化入口

```{literalinclude} ../../../../tests/YCSB-C/ycsbc.cc
:language: cpp
:start-after: void PrepareShmEnv
:end-before: auto build_dbs
```

### BwTree：`nt<T>` 的高密度使用位置

```{literalinclude} ../../../../ds/BwTree/src/bwtree.h
:language: cpp
:start-after: // This must be declared before all include directives
:end-before: atomic_stack.h
```

## 贡献建议

- 每次新增宏分支（如 `NO_CC` / `QUEUE_SIM_CAS`）时，同步补一段「行为差异 + 验证方法」
- 每次新增 allocator/queue 能力时，至少补一段「最小可复现代码」
- 每次改动并发路径时，补齐一个 correctness case 与一个性能回归 case
