# Message Queue 与 Dispatcher（跨机消息路径）

当你的模块需要跨机 free、远端事件通知或自定义消息处理时，应使用 `shm-lib` 的 queue/dispatcher 入口。

## 1. 公共入口（头文件）

位置：`shm-lib/include/shm/mempool.h`

- `initialize_shm_and_queue()`
- `initialize_comm(int machine_no)`
- `add_dispatcher(msg_type_t type, MsgHandler handler)`
- `marker_free_cross_machine(memkind_t kind, void* ptr)`
- `get_ptr_machine_index(void* ptr)`

## 2. 消息类型定义

```{literalinclude} ../../../../shm-lib/include/msg/msg_queue.h
:language: cpp
:start-after: typedef enum msg_type
:end-before: class MsgQueue
```

## 3. 推荐初始化流程

1. 完成本地共享内存初始化  
2. 调用 `initialize_shm_and_queue()` 创建消息基础设施  
3. 调用 `initialize_comm(machine_no)` 建立机器编号与通信上下文  
4. 通过 `add_dispatcher(...)` 注册类型到 handler 的映射  
5. 在回收路径中调用 `marker_free_cross_machine(...)`

## 4. 与 `initialize_shm_related()` 的关系

`initialize_shm_related(machine_no)` 是便捷入口，但在不同宏分支下可能携带内部实现差异。  
如果你想明确控制初始化阶段，建议直接按上面的公共 API 分步调用。

## 5. 常见错误

- 忘记注册某类消息 dispatcher，导致消息堆积或静默丢弃
- machine 编号不一致，导致路由错误
- 本地 free 与跨机 free 混用，造成双重释放风险

## 6. 最小验证方案

- 单机多进程：验证 dispatcher 是否被正确触发
- 双机场景：验证 `get_ptr_machine_index(ptr)` 与实际归属一致
- 压测：持续触发远端回收，观察 queue backlog 与处理延迟
