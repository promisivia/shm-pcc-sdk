---
orphan: true
---

# Components

这里提供“怎么用组件”的文档入口，面向希望直接在共享内存/CXL 平台上落地系统原型的读者：

- 数据结构：如何初始化共享内存分配器、如何选择/接入 `ds/**` 的容器
- 通信/RPC：如何使用共享内存消息队列与多机 master/follower 协作逻辑
- Apps：以 YCSB-C 为主样例，给出可复现的构建与运行路径

```{toctree}
:maxdepth: 2

data_structures
rpc_comms
apps
```
