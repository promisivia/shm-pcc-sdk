# Queues & Threading

本页以 YCSB-C 的线程池实现为主线，解释 `shm-lib` 队列接口的常用用法与约束，便于你在自己的系统里复用同类结构（任务分发/负载均衡/异步执行）。

## 进程内并发队列：`MPMCQueue<T>`

`shm-lib/include/msg/mpmc_queue.h`：

```{literalinclude} ../../../../shm-lib/include/msg/mpmc_queue.h
:language: cpp
:start-after: template <typename T>
:end-before: "#else"
```

## 真实用法：YCSB-C `ThreadPool`

```{literalinclude} ../../../../tests/YCSB-C/include/db/thread_pool.h
:language: cpp
```

## 说明

- `MPMCQueue<std::function<void()>>` 是一种简单通用的任务队列实现；高频路径上要注意捕获/拷贝成本。
- 如果启用特定宏（例如持久化/flush 路径），队列实现细节会变化；建议以你的编译配置为准做性能评估。

## 什么时候用 `MPMCQueue`，什么时候看 `MsgQueue`？

- `MPMCQueue<T>`：进程内多生产者/多消费者任务队列（YCSB 的线程池就是这个模式）。
- `MsgQueue`/dispatcher/collector：当你要做跨机/跨进程的消息收发与回调注册时，再进入这一套（见 API：{doc}`../api/queues` 与 {doc}`../api/comms`）。
